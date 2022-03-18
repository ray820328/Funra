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

#ifndef CPL_INIT_H
#define CPL_INIT_H

#include <cpl_macros.h>


CPL_BEGIN_DECLS

#define CPL_INIT_DEFAULT 0
#define CPL_DESCRIPTION_DEFAULT 0

void cpl_init(unsigned);
const char * cpl_get_description(unsigned);
void cpl_end(void);

CPL_END_DECLS

#endif /* CPL_INIT_H */
