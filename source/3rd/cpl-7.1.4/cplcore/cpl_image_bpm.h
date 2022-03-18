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

#ifndef CPL_IMAGE_BPM_H
#define CPL_IMAGE_BPM_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image.h"
#include "cpl_bivector.h"
#include "cpl_mask.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

/**
 * @ingroup cpl_image
 *
 * @brief The special values that can be rejected
 * They are a bit-field and can be combined with bitwise or.
 */

enum _cpl_value_ {

/* No entry has the value 1 which makes the (mis)use of logical or detectable */

    /**
     * Not-a-Number (NaN)
     * @hideinitializer
     */
    CPL_VALUE_NAN        =  1 << 1,
    /**
     * Plus Infinity
     * @hideinitializer
     */
    CPL_VALUE_PLUSINF    =  1 << 2,
    /**
     * Minus Infinity
     * @hideinitializer
     */
    CPL_VALUE_MINUSINF   =  1 << 3,
    /**
     * Zero
     * @hideinitializer
     */
    CPL_VALUE_ZERO       =  1 << 4,
    /**
     * Infinity with any sign
     * @hideinitializer
     */
    CPL_VALUE_INF        =  CPL_VALUE_PLUSINF | CPL_VALUE_MINUSINF,
    /**
     * NaN or infinity with any sign
     * @hideinitializer
     */
    CPL_VALUE_NOTFINITE  =  CPL_VALUE_INF | CPL_VALUE_NAN
};


/**
 * @ingroup cpl_image
 *
 * @brief
 *   The CPL special value. It is a bit field.
 */
typedef enum _cpl_value_ cpl_value;



/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

/* Info on the bad pixels */
cpl_size cpl_image_count_rejected(const cpl_image *);
int cpl_image_is_rejected(const cpl_image *, cpl_size, cpl_size);

/* To modify an image's bad pixel map */
cpl_error_code cpl_image_reject(cpl_image *, cpl_size, cpl_size);
cpl_error_code cpl_image_accept(cpl_image *, cpl_size, cpl_size);
cpl_error_code cpl_image_accept_all(cpl_image *);
cpl_error_code cpl_image_reject_from_mask(cpl_image *, const cpl_mask *);
cpl_error_code cpl_image_reject_value(cpl_image *, cpl_value);

CPL_END_DECLS

#endif 
