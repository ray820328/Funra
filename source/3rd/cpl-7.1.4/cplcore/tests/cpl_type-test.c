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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_type.h"
#include "cpl_test.h"

#include <string.h>
#include <complex.h>

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    typedef struct {
        cpl_type type;
        size_t size;
    } typelist;

    const typelist supported[] =  { {CPL_TYPE_INVALID, 0},
                                    {CPL_TYPE_STRING, sizeof(char)},
                                    {CPL_TYPE_CHAR,   sizeof(char)},
                                    {CPL_TYPE_UCHAR,  sizeof(unsigned char)},
                                    {CPL_TYPE_BOOL,   sizeof(cpl_boolean)},
                                    {CPL_TYPE_SHORT,  sizeof(short)},
                                    {CPL_TYPE_USHORT, sizeof(unsigned short)},
                                    {CPL_TYPE_INT,    sizeof(int)},
                                    {CPL_TYPE_UINT,   sizeof(unsigned int)},
                                    {CPL_TYPE_LONG,   sizeof(long int)},
                                    {CPL_TYPE_ULONG,  sizeof(unsigned long int)},
                                    {CPL_TYPE_LONG_LONG, sizeof(long long int)},
                                    {CPL_TYPE_SIZE,   sizeof(cpl_size)},
                                    {CPL_TYPE_FLOAT,  sizeof(float)},
                                    {CPL_TYPE_DOUBLE, sizeof(double)},
                                    {CPL_TYPE_FLOAT_COMPLEX,
                     sizeof(float complex)},
                                    {CPL_TYPE_DOUBLE_COMPLEX,
                     sizeof(double complex)},
                                    {CPL_TYPE_UNSPECIFIED, 0},
                                    {CPL_TYPE_POINTER, sizeof(void*)}};

    const int nsupported = (int)(sizeof(supported) / sizeof(typelist));
    const char * empty;
    int i;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    cpl_test_zero(CPL_FALSE);
    cpl_test_noneq(CPL_FALSE, CPL_TRUE);

    for (i = 0; i < nsupported; i++) {
        const char * name = cpl_type_get_name(supported[i].type);

        cpl_msg_info(cpl_func, "Testing type: %s (size=%u)",
                     name, (unsigned)supported[i].size);

        cpl_test_eq(cpl_type_get_sizeof(supported[i].type), supported[i].size);

        cpl_test_eq(cpl_type_get_sizeof(supported[i].type | CPL_TYPE_FLAG_ARRAY),
                    supported[i].size);

        cpl_test_nonnull(name);

        if (supported[i].size != 0) {
            const char * array = cpl_type_get_name(supported[i].type
                                                   | CPL_TYPE_POINTER);
            cpl_test_nonnull(array);

            if (array != NULL) {

                if (name != NULL) {
                    /* Array name starts with type name, e.g. "double array" */
                    cpl_test_eq_string(strstr(array, name), array);
                }

                if (supported[i].type != CPL_TYPE_POINTER) {
                    /* And also contains the sub-string " array" */
                    cpl_test_nonnull(strstr(array, " array"));
                } else {
                    cpl_test_null(strstr(array, " array"));
                }
            }
        }

        /* Get a new name by combining with flag array */
        /* Cannot be done for pointer, nor for char/string since those two
           are defined using this combination */
        if (supported[i].size != 0 && supported[i].type != CPL_TYPE_POINTER
            && supported[i].type != CPL_TYPE_STRING
            && supported[i].type != CPL_TYPE_CHAR) {
            const char * array = cpl_type_get_name(supported[i].type
                                                   | CPL_TYPE_FLAG_ARRAY);
            cpl_test_nonnull(array);

            if (array != NULL) {

                if (name != NULL) {
                    /* Array name starts with type name, e.g. "double array" */
                    cpl_test_eq_string(strstr(array, name), array);
                }

                /* And also contains the sub-string " array" */
                cpl_test_nonnull(strstr(array, " array"));
            }
        }
    }

    cpl_test_nonnull(cpl_type_get_name(CPL_TYPE_FLAG_ARRAY));

    /* Unsupported combination */
    empty = cpl_type_get_name(CPL_TYPE_POINTER | CPL_TYPE_FLAG_ARRAY);
    cpl_test_error(CPL_ERROR_INVALID_TYPE);
    cpl_test_nonnull(empty);
    if (empty != NULL) cpl_test_zero(*empty);

    return cpl_test_end(0);
}
