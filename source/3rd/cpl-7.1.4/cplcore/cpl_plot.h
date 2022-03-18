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

#ifndef CPL_PLOT_H
#define CPL_PLOT_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_error.h"
#include "cpl_vector.h"
#include "cpl_bivector.h"
#include "cpl_table.h"
#include "cpl_image.h"
#include "cpl_mask.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                              Function prototypes
 -----------------------------------------------------------------------------*/

cpl_error_code cpl_plot_vector(const char *, const char *, const char *, 
                               const cpl_vector *);

cpl_error_code cpl_plot_vectors(const char *, const char *, const char *, 
                                const cpl_vector **, cpl_size);

cpl_error_code cpl_plot_bivector(const char *, const char *, const char *, 
                                 const cpl_bivector *);

cpl_error_code cpl_plot_bivectors(const char *, const char **, const char *, 
                                  const cpl_bivector **, cpl_size);

cpl_error_code cpl_plot_mask(const char *, const char *, const char *, 
                             const cpl_mask *);

cpl_error_code cpl_plot_image(const char *, const char *, const char *, 
                              const cpl_image *);

cpl_error_code cpl_plot_image_row(const char *, const char *, const char *, 
                                  const cpl_image *, cpl_size, cpl_size,
                                  cpl_size);

cpl_error_code cpl_plot_image_col(const char *, const char *, const char *, 
                                  const cpl_image *, cpl_size, cpl_size,
                                  cpl_size);

cpl_error_code cpl_plot_column(const char *, const char *, const char *, 
                               const cpl_table *, const char *, const char *);

cpl_error_code cpl_plot_columns(const char *, const char *, const char *, 
                                const cpl_table *, const char **, cpl_size);

CPL_END_DECLS

#endif
