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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef CPL_WLCALIB_H
#define CPL_WLCALIB_H

/*-----------------------------------------------------------------------------
                                Include
 -----------------------------------------------------------------------------*/

#include <cpl_polynomial.h>
#include <cpl_bivector.h>

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

typedef struct cpl_wlcalib_slitmodel_ cpl_wlcalib_slitmodel;

/*-----------------------------------------------------------------------------
                                Functions prototypes
 -----------------------------------------------------------------------------*/

/* Functions to create and destroy a slitmodel */
cpl_wlcalib_slitmodel * cpl_wlcalib_slitmodel_new(void) CPL_ATTR_ALLOC;
void cpl_wlcalib_slitmodel_delete(cpl_wlcalib_slitmodel *);

/* Functions to initialize the slitmodel */
cpl_error_code cpl_wlcalib_slitmodel_set_wslit(cpl_wlcalib_slitmodel *, double);
cpl_error_code cpl_wlcalib_slitmodel_set_wfwhm(cpl_wlcalib_slitmodel *, double);
cpl_error_code cpl_wlcalib_slitmodel_set_threshold(cpl_wlcalib_slitmodel *,
                                                   double);
cpl_error_code cpl_wlcalib_slitmodel_set_catalog(cpl_wlcalib_slitmodel *,
                                                 cpl_bivector *);

/* Functions to make a model spectrum using the slitmodel */
cpl_error_code cpl_wlcalib_fill_line_spectrum(cpl_vector *, void *,
                                              const cpl_polynomial *);

cpl_error_code cpl_wlcalib_fill_logline_spectrum(cpl_vector *, void *,
                                                 const cpl_polynomial *);

cpl_error_code cpl_wlcalib_fill_line_spectrum_fast(cpl_vector *, void *,
                                                   const cpl_polynomial *);

cpl_error_code cpl_wlcalib_fill_logline_spectrum_fast(cpl_vector *, void *,
                                                      const cpl_polynomial *);

/* Function to calibrate a spectrum using a spectral model */
cpl_error_code cpl_wlcalib_find_best_1d(cpl_polynomial       *,
                                        const cpl_polynomial *,
                                        const cpl_vector     *,
                                        void                 *,
                                        cpl_error_code      (*)
                                        (cpl_vector *, void *,
                                         const cpl_polynomial *),
                                        const cpl_vector     *,
                                        cpl_size,
                                        cpl_size,
                                        double               *,
                                        cpl_vector           *);

#endif
