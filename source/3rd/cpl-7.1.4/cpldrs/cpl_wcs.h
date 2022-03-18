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

#ifndef CPL_WCS_H
#define CPL_WCS_H

#include "cpl_error.h"
#include "cpl_propertylist.h"
#include "cpl_matrix.h"
#include "cpl_array.h"

/* Definition of WCS structure */

typedef struct _cpl_wcs_ cpl_wcs;

/* Enumerate the modes available for the convert routine */

enum cpl_wcs_trans_mode {
    CPL_WCS_PHYS2WORLD,
    CPL_WCS_WORLD2PHYS,
    CPL_WCS_PHYS2STD,
    CPL_WCS_WORLD2STD};

typedef enum cpl_wcs_trans_mode cpl_wcs_trans_mode;

/* Enumerate the modes available for the plate solution routine */

enum cpl_wcs_platesol_fitmode {
    CPL_WCS_PLATESOL_6,
    CPL_WCS_PLATESOL_4};

typedef enum cpl_wcs_platesol_fitmode cpl_wcs_platesol_fitmode;

enum cpl_wcs_platesol_outmode {
    CPL_WCS_MV_CRVAL,
    CPL_WCS_MV_CRPIX};

typedef enum cpl_wcs_platesol_outmode cpl_wcs_platesol_outmode;

/* Macro definitions */

/*----------------------------------------------------------------------------*/
/**
   @hideinitializer
   @ingroup cpl_wcs
   @brief  A regular expression that matches the FITS keys used for WCS
 */
/*----------------------------------------------------------------------------*/
#define CPL_WCS_REGEXP "WCSAXES|WCSNAME|(PC|CD|PV|PS)[0-9]+_[0-9]+|"    \
    "C(RVAL|RPIX|DELT|TYPE|UNIT|RDER|SYER)[0-9]+"

/* Function prototypes */

CPL_BEGIN_DECLS

cpl_wcs *cpl_wcs_new_from_propertylist(const cpl_propertylist *plist)
    CPL_ATTR_ALLOC;
void cpl_wcs_delete(cpl_wcs *wcs);
cpl_error_code cpl_wcs_convert(const cpl_wcs *wcs, const cpl_matrix *from,
                               cpl_matrix **to, cpl_array **status,
                               cpl_wcs_trans_mode transform);
cpl_error_code cpl_wcs_platesol(const cpl_propertylist *ilist,
                                const cpl_matrix *cel,
                                const cpl_matrix *xy, int niter, float thresh,
                                cpl_wcs_platesol_fitmode fitmode,
                                cpl_wcs_platesol_outmode outmode,
                                cpl_propertylist **olist);

/* Accessor routines */

int cpl_wcs_get_image_naxis(const cpl_wcs *wcs);
const cpl_array *cpl_wcs_get_image_dims(const cpl_wcs *wcs);
const cpl_array *cpl_wcs_get_crval(const cpl_wcs *wcs);
const cpl_array *cpl_wcs_get_crpix(const cpl_wcs *wcs);
const cpl_array *cpl_wcs_get_ctype(const cpl_wcs *wcs);
const cpl_array *cpl_wcs_get_cunit(const cpl_wcs *wcs);
const cpl_matrix *cpl_wcs_get_cd(const cpl_wcs *wcs);

CPL_END_DECLS

#endif /* CPL_WCS_H */

