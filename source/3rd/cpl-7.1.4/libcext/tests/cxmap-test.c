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

#include <stdio.h>
#include <string.h>

#include "cxmemory.h"
#include "cxmessages.h"
#include "cxmap.h"


static cxbool
cx_test_map_less_char(cxcptr a, cxcptr b)
{

    cxchar _a = *((const cxchar *)a);
    cxchar _b = *((const cxchar *)b);

    return _a - _b < 0 ? TRUE : FALSE;

}


#ifdef CX_TEST_MAP_GREATER_CHAR
static cxbool
cx_test_map_greater_char(cxcptr a, cxcptr b)
{

    cxchar _a = *((const cxchar *)a);
    cxchar _b = *((const cxchar *)b);

    return _a - _b > 0 ? TRUE : FALSE;

}
#endif

#ifdef CX_TEST_MAP_DUMP
static void
cx_test_map_dump(cx_map *map, cxint fmt)
{

    cxint i;
    cx_map_iterator pos;


    fprintf(stderr, "map at address %p:\n", map);

    i = 0;
    pos = cx_map_begin(map);

    while (pos != cx_map_end(map)) {
        fprintf(stderr, "pair %d at address %p has value ", i + 1,
                pos);

        switch (fmt) {
            case 0:
                fprintf(stderr, "%s", (cxchar *)cx_map_get_value(map, pos));
                break;

            case 1:
                fprintf(stderr, "%c",
                        *((cxint *)cx_map_get_value(map, pos)));
                break;

            default:
                fprintf(stderr, "at address %p of unknown type",
                        cx_map_get_value(map, pos));
                break;
        }

        fprintf(stderr, "\n");

        pos = cx_map_next(map, pos);
        i++;
    }

    return;

}
#endif

int
main(void)
{

    cxchar letters[] = "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    cxsize i;

    cx_map *map;
    cx_map_iterator pos;


    /*
     * Note:
     *   The tests for maps only cover reimplementations or additions
     *   compared to the cx_tree base class, since all other tests are
     *   already done for cx_tree.
     */


    /*
     * Test 1: Create a map from the characters letters[], update the
     *         value of a pair through its key. Verify data and tree
     *         structure.
     */

    map = cx_map_new(cx_test_map_less_char, NULL, NULL);

    for (i = 0; i < strlen(letters); i++)
        cx_map_insert(map, &letters[i], &letters[i]);

    cx_assert(cx_tree_verify(map) != FALSE);

    cx_map_put(map, &letters[25], &letters[23]);

    pos = cx_map_lower_bound(map, &letters[25]);
    cx_assert(cx_tree_verify(map) != FALSE);
    cx_assert(cx_map_upper_bound(map, &letters[25]) == cx_map_next(map, pos));
    cx_assert(*((cxchar *)cx_map_get_value(map, pos)) == letters[23]);


    /*
     * Test 2: Check that the pair modified in the previous test can be
     *         properly retrieved again.
     */

    cx_assert(*((cxchar *)cx_map_get(map, &letters[25])) == letters[23]);

    cx_map_delete(map);


    /*
     * All tests succeeded
     */

    return 0;

}
