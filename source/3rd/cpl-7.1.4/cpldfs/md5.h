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

#ifndef MD5_H
#define MD5_H

typedef unsigned int word32;

struct MD5Context {
    word32 buf[4];
    word32 bits[2];
    unsigned char in[64];
};

static void MD5Init(struct MD5Context * restrict context)
    CPL_ATTR_NONNULL;
static void MD5Update(struct MD5Context * restrict context,
                      unsigned char const * restrict buf,
                      unsigned len)
    CPL_ATTR_NONNULL;
static void MD5Final(unsigned char * restrict digest,
                     struct MD5Context * restrict context)
    CPL_ATTR_NONNULL;

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#endif 
