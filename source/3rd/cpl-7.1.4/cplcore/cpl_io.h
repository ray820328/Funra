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

#ifndef CPL_IO_H
#define CPL_IO_H

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include <cpl_macros.h>

CPL_BEGIN_DECLS

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_io I/O
 *
 * This module provides definitions related to I/O. The actual I/O functions
 * are defined in the respective CPL modules.
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_io.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/

/**@{*/

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/**
 * 
 * @brief These are the file I/O modes.
 *
 * For the compression modes, 
 * see http://heasarc.nasa.gov/docs/software/fitsio/compression.html
 */
/*----------------------------------------------------------------------------*/

enum _cpl_io_type_ {

    /* 
     * More modes may be added in the future. To support future combination of
     * different modes (using bit-wise or) no mode has the value 1, since this
     * makes the (mis)use of logical or detectable.
     */

    /** @hideinitializer */
    CPL_IO_CREATE = ((unsigned)1 << 1),
    /**< Overwrite the file, if it already exists. */

    /** @hideinitializer */
    CPL_IO_EXTEND = ((unsigned)1 << 2),
    /**< Append a new extension to the file. */

    /** @hideinitializer */
    CPL_IO_APPEND = ((unsigned)1 << 3),
    /**< Append to the last data unit of the file. */

    /** @hideinitializer */
    CPL_IO_COMPRESS_GZIP = ((unsigned)1 << 4),
    /**< Use FITS tiled-image compression with GZIP algorithm. */

    /** @hideinitializer */
    CPL_IO_COMPRESS_RICE = ((unsigned)1 << 5),
    /**< Use FITS tiled-image compression with RICE algorithm. */

    /** @hideinitializer */
    CPL_IO_COMPRESS_HCOMPRESS = ((unsigned)1 << 6),
    /**< Use FITS tiled-image compression with HCOMPRESS algorithm. */

    /** @hideinitializer */
    CPL_IO_COMPRESS_PLIO = ((unsigned)1 << 7),
    /**< Use FITS tiled-image compression with PLIO algorithm. */

    /** @hideinitializer */
    CPL_IO_MAX = ((unsigned)1 << 8),
    /**< Reserved for internal CPL usage. */

    /** @hideinitializer */
    CPL_IO_DEFAULT  = ((unsigned)CPL_IO_CREATE)
    /**< Deprecated, kept only for backwards compatibility */

};

/**
 * @brief
 *   The file I/O modes.
 */

typedef enum _cpl_io_type_ cpl_io_type;



/**
   @deprecated Use CPL_TYPE_UCHAR
*/
#define CPL_BPP_8_UNSIGNED  CPL_TYPE_UCHAR

/**
   @deprecated Use CPL_TYPE_SHORT
*/
#define CPL_BPP_16_SIGNED   CPL_TYPE_SHORT

/**
   @deprecated Use CPL_TYPE_USHORT
*/
#define CPL_BPP_16_UNSIGNED CPL_TYPE_USHORT

/**
   @deprecated Use CPL_TYPE_INT
*/
#define CPL_BPP_32_SIGNED   CPL_TYPE_INT

/**
   @deprecated Use CPL_TYPE_FLOAT
*/
#define CPL_BPP_IEEE_FLOAT  CPL_TYPE_FLOAT

/**
   @deprecated Use CPL_TYPE_DOUBLE
*/
#define CPL_BPP_IEEE_DOUBLE CPL_TYPE_DOUBLE
/**
   @deprecated Use cpl_type
*/
#define cpl_type_bpp        cpl_type

/**@}*/

CPL_END_DECLS

#endif /* CPL_IO_H */
