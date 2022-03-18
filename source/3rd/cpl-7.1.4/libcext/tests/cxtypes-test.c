/*
 * This file is part of the ESO C Extension Library
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

#undef CX_DISABLE_ASSERT
#undef CX_LOG_DOMAIN

#include "cxmemory.h"
#include "cxmessages.h"
#include "cxtypes.h"


int
main(void)
{

    /*
     * Test 1: Verify the size of the cxtypes.
     */

    cx_assert(sizeof(cxint8)   == 1);
    cx_assert(sizeof(cxuint8)  == 1);
    cx_assert(sizeof(cxint16)  == 2);
    cx_assert(sizeof(cxuint16) == 2);
    cx_assert(sizeof(cxint32)  == 4);
    cx_assert(sizeof(cxuint32) == 4);
    cx_assert(sizeof(cxint64)  == 8);
    cx_assert(sizeof(cxuint64) == 8);


    /*
     * All tests succeeded
     */

    return 0;

}
