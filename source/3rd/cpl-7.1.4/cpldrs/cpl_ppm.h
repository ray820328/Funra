/*
 * This file is part of the VIMOS Pipeline
 * Copyright (C) 2002-2017 European Southern Observatory
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef CPL_PPM_H
#define CPL_PPM_H

#include <cpl_bivector.h>
#include <cpl_array.h>

CPL_BEGIN_DECLS

cpl_bivector *cpl_ppm_match_positions(const cpl_vector *, const cpl_vector *,
                                      double,  double, double,
                                      cpl_array **, cpl_array **)
    CPL_ATTR_ALLOC;

cpl_array *cpl_ppm_match_points(const cpl_matrix *, cpl_size, double,
                                const cpl_matrix *, cpl_size, double,
                                double, double,
                                cpl_matrix **, cpl_matrix **,
                                double *, double *) CPL_ATTR_ALLOC;

CPL_END_DECLS

#endif   /* CPL_PPM_H */
