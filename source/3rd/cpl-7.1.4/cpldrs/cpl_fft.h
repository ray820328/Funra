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

#ifndef CPL_FFT_H
#define CPL_FFT_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_imagelist.h"

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                   New types
 -----------------------------------------------------------------------------*/

/**
 * @ingroup cpl_fft
 * @brief The supported values of the CPL fft mode.
 */

enum _cpl_fft_mode_ {

    /**
     * The forward transform
     * @hideinitializer
     */
    CPL_FFT_FORWARD        =  1 << 1,
    /**
     * The backward transform
     * @hideinitializer
     */
    CPL_FFT_BACKWARD       =  1 <<  2,

    /**
     * Transform without scaling (has no effect on forward transform)
     * @hideinitializer
     */
    CPL_FFT_NOSCALE        =  1 <<  3,

    /**
     * Spend time finding the best transform
     * @hideinitializer
     */
    CPL_FFT_FIND_MEASURE   =  1 <<  4,

    /**
     * Spend more time finding the best transform
     * @hideinitializer
     */
    CPL_FFT_FIND_PATIENT   =  1 <<  5,

    /**
     * Spend even more time finding the best transform
     * @hideinitializer
     */
    CPL_FFT_FIND_EXHAUSTIVE =  1 <<  6

};


/**
 * @ingroup cpl_fft
 * @brief The CPL fft mode.
 */
typedef enum _cpl_fft_mode_ cpl_fft_mode;

/*-----------------------------------------------------------------------------
                            Function prototypes
 -----------------------------------------------------------------------------*/

cpl_error_code cpl_fft_image(cpl_image *, const cpl_image *, cpl_fft_mode);
cpl_error_code cpl_fft_imagelist(cpl_imagelist *, const cpl_imagelist *,
                                 cpl_fft_mode);
CPL_END_DECLS

#endif /* CPL_FFT_H */

