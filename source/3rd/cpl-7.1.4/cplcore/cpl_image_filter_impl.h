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

#ifndef CPL_IMAGE_FILTER_IMPL_H
#define CPL_IMAGE_FILTER_IMPL_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image_filter.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/
void
cpl_image_filter_fill_chess_double(double *in_larger, const double *in,
                                   unsigned Nx_larger, unsigned Ny_larger, 
                                   unsigned Nx, unsigned Ny, 
                                   unsigned rx, unsigned ry);

void
cpl_image_filter_fill_chess_float(float *in_larger, const float *in,
                                  unsigned Nx_larger, unsigned Ny_larger, 
                                  unsigned Nx, unsigned Ny, 
                                  unsigned rx, unsigned ry);

void
cpl_image_filter_fill_chess_int(int *in_larger, const int *in,
                                unsigned Nx_larger, unsigned Ny_larger, 
                                unsigned Nx, unsigned Ny, 
                                unsigned rx, unsigned ry);

CPL_END_DECLS

#endif 

