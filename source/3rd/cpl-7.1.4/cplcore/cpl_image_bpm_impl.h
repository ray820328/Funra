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

#ifndef CPL_IMAGE_BPM_IMPL_H
#define CPL_IMAGE_BPM_IMPL_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image_bpm.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/
void cpl_image_accept_(cpl_image *,
                       cpl_size,
                       cpl_size)
    CPL_ATTR_NONNULL;

void cpl_image_reject_(cpl_image *,
                       cpl_size,
                       cpl_size)
    CPL_ATTR_NONNULL;

CPL_END_DECLS

#endif 

