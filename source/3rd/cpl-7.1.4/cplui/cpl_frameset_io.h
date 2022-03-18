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

#ifndef CPL_FRAMESET_IO_H
#define CPL_FRAMESET_IO_H

#include <cpl_macros.h>
#include <cpl_frameset.h>
#include <cpl_imagelist.h>
#include <cpl_type.h>

CPL_BEGIN_DECLS

/* Load an image list from a frame set */
cpl_imagelist * cpl_imagelist_load_frameset(const cpl_frameset *, cpl_type, 
                                            cpl_size, cpl_size) CPL_ATTR_ALLOC;

CPL_END_DECLS

#endif
