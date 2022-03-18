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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CPL_WLCALIB_IMPL_H
#define CPL_WLCALIB_IMPL_H

/*-----------------------------------------------------------------------------
                                Include
 -----------------------------------------------------------------------------*/

#include "cpl_wlcalib.h"


/*-----------------------------------------------------------------------------
                                Functions prototypes
 -----------------------------------------------------------------------------*/

/* Cost counting functions for the slitmodel */
cpl_error_code cpl_wlcalib_slitmodel_add_cost(cpl_wlcalib_slitmodel *,
                                              cpl_size);
cpl_error_code cpl_wlcalib_slitmodel_add_xcost(cpl_wlcalib_slitmodel *,
                                              cpl_size);
cpl_error_code cpl_wlcalib_slitmodel_add_ulines(cpl_wlcalib_slitmodel *,
                                                cpl_size);

/* Acessors of the slitmodel */
double cpl_wlcalib_slitmodel_get_wslit(const cpl_wlcalib_slitmodel *);
double cpl_wlcalib_slitmodel_get_wfwhm(const cpl_wlcalib_slitmodel *);
double cpl_wlcalib_slitmodel_get_threshold(const cpl_wlcalib_slitmodel *);
cpl_size cpl_wlcalib_slitmodel_get_cost(const cpl_wlcalib_slitmodel *);
cpl_size cpl_wlcalib_slitmodel_get_xcost(const cpl_wlcalib_slitmodel *);
cpl_size cpl_wlcalib_slitmodel_get_ulines(const cpl_wlcalib_slitmodel *);

cpl_bivector * cpl_wlcalib_slitmodel_get_catalog(cpl_wlcalib_slitmodel *);
const cpl_bivector *
    cpl_wlcalib_slitmodel_get_catalog_const(const cpl_wlcalib_slitmodel *);
cpl_bivector * cpl_wlcalib_slitmodel_unset_catalog(cpl_wlcalib_slitmodel *);

#endif
