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
#include "cxlist.h"


static cxint
cx_test_list_compare1(cxcptr a, cxcptr b)
{

    cxint _a = *((const cxint *)a);
    cxint _b = *((const cxint *)b);

    return _a - _b;

}


static cxint
cx_test_list_compare2(cxcptr a, cxcptr b)
{

    cxint _a = *((const cxint *)a);
    cxint _b = *((const cxint *)b);

    return _b - _a;

}


#ifdef CX_TEST_LIST_DUMP
static void
cx_test_list_dump(cx_list *list, cxint fmt)
{

    cxint i;
    cx_list_iterator pos;


    fprintf(stderr, "list at start address %p:\n", list);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        fprintf(stderr, "list element %d at address %p has value ", i + 1,
                pos);

        switch (fmt) {
            case 0:
                fprintf(stderr, "%s", (cxchar *)cx_list_get(list, pos));
                break;

            case 1:
                fprintf(stderr, "%d", *((cxint *)cx_list_get(list, pos)));
                break;

            default:
                fprintf(stderr, "at address %p of unknown type",
                        cx_list_get(list, pos));
                break;
        }

        fprintf(stderr, "\n");

        pos = cx_list_next(list, pos);
        i++;
    }

    return;

}
#endif

int
main(void)
{

    cx_list *list, *_list;
    cx_list_iterator pos, first, last;

    cxchar letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    cxint i, ival;
    cxint num1[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    cxint num2[10] = {8, 9, 7, 0, 3, 2, 5, 1, 4, 6};



    /*
     * Test 1: Create a list, check that it is a valid empty list and
     *         destroy it again.
     */

    list = cx_list_new();
    cx_assert(list != NULL);

    cx_assert(cx_list_empty(list));
    cx_assert(cx_list_size(list) == 0);
    cx_assert(cx_list_begin(list) == cx_list_end(list));

    cx_list_delete(list);


    /*
     * Test 2: Create a list and append the numbers num1, check that
     *         the data were properly appended, thereby verifying that
     *         we can properly step through the list. Finally, step
     *         through list backwards starting at the end.
     */

    list = cx_list_new();

    for (i = 0; i < 10; i++)
        cx_list_push_back(list, &num1[i]);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) == i);

        pos = cx_list_next(list, pos);
        i++;
    }

    i = 0;
    pos = cx_list_previous(list, cx_list_end(list));

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) == (9 - i));

        pos = cx_list_previous(list, pos);
        i++;
    }

    cx_list_delete(list);


    /*
     * Test 3: Same as test 2, but insert the numbers as head of the list.
     */

    list = cx_list_new();

    for (i = 0; i < 10; i++)
        cx_list_push_front(list, &num1[i]);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) == (9 - i));

        pos = cx_list_next(list, pos);
        i++;
    }


    /*
     * Test 4: Use the list created in the previous test for removing
     *         elements from the head and the tail. Check the data.
     */

    first = cx_list_next(list, cx_list_begin(list));

    ival = *((cxint *)cx_list_pop_front(list));
    cx_assert(ival == 9);

    ival = *((cxint *)cx_list_pop_back(list));
    cx_assert(ival == 0);

    i = 1;
    pos = cx_list_begin(list);
    cx_assert(pos == first);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) == (9 - i));

        pos = cx_list_next(list, pos);
        i++;
    }


    /*
     * Test 5: Use the list from the previous test and remove all
     *         elements from the list. Check that the list is empty.
     */

    cx_assert(!cx_list_empty(list));

    cx_list_clear(list);

    cx_assert(cx_list_empty(list));
    cx_assert(cx_list_size(list) == 0);

    cx_list_delete(list);


    /*
     * Test 6: Create lists different sizes using the numbers num1 as
     *         elements and check that the list size is properly reported.
     */

    /* 10 elements */

    list = cx_list_new();

    for (i = 0; i < 10; i++)
        cx_list_push_back(list, &num1[i]);

    cx_assert(!cx_list_empty(list));
    cx_assert(cx_list_size(list) == 10);

    cx_list_clear(list);


    /* 5 elements */

    for (i = 0; i < 5; i++)
        cx_list_push_back(list, &num1[i]);

    cx_assert(!cx_list_empty(list));
    cx_assert(cx_list_size(list) == 5);

    cx_list_clear(list);


    /* 1 element */

    for (i = 0; i < 1; i++)
        cx_list_push_back(list, &num1[i]);

    cx_assert(!cx_list_empty(list));
    cx_assert(cx_list_size(list) == 1);

    cx_list_delete(list);


    /*
     * Test 7: Create a list from the numbers num1. Remove the element
     *         at the 6th position from the list and verify the data.
     */

    list = cx_list_new();

    for (i = 0; i < 10; i++)
        cx_list_push_back(list, &num1[i]);

    pos = cx_list_begin(list);
    for (i = 0; i < 5; i++)
        pos = cx_list_next(list, pos);

    ival = *((cxint *)cx_list_extract(list, pos));
    cx_assert(ival == 5);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);

        if (i == 5)
            i++;

        cx_assert(*((cxint *)data) == i);

        pos = cx_list_next(list, pos);
        i++;

    }


    /*
     * Test 7: Reinsert the number num1[5] as the 6th list element
     *         and verify the data.
     */


    pos = cx_list_begin(list);

    for (i = 0; i < 5; i++)
        pos = cx_list_next(list, pos);

    pos = cx_list_insert(list, pos, &num1[5]);
    cx_assert(*((cxint *)cx_list_get(list, pos)) == 5);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) == i);

        pos = cx_list_next(list, pos);
        i++;

    }


    /*
     * Test 8: Assign the number num1[4] to the 6th and 9th list position.
     *         Verify the data.
     */

    pos = cx_list_begin(list);

    for (i = 0; i < 5; i++)
        pos = cx_list_next(list, pos);

    ival = *((cxint *)cx_list_assign(list, pos, &num1[4]));
    cx_assert(ival == 5);

    for (i = 5; i < 8; i++)
        pos = cx_list_next(list, pos);

    ival = *((cxint *)cx_list_assign(list, pos, &num1[4]));
    cx_assert(ival == 8);


    /*
     * Test 9: Remove consecutive duplicates from the list. This should
     *         happen for list position 6 only. Verify the data.
     */

    cx_list_unique(list, cx_test_list_compare1);
    cx_assert(cx_list_size(list) == 9);

    pos = cx_list_begin(list);

    for (i = 0; i < 5; i++)
        pos = cx_list_next(list, pos);

    ival = *((cxint *)cx_list_get(list, pos));
    cx_assert(ival == 6);


    /*
     * Test 10: Remove all list elements with value num1[4] from the list.
     *          Verify the data.
     */

    cx_list_remove(list, &num1[4]);
    cx_assert(cx_list_size(list) == 7);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) != num1[4]);

        pos = cx_list_next(list, pos);
        i++;

    }

    cx_list_delete(list);


    /*
     * Test 11: Create a list from the numbers num1, reverse the list and
     *          check the data.
     */

    list = cx_list_new();

    for (i = 0; i < 10; i++)
        cx_list_push_back(list, &num1[i]);

    cx_list_reverse(list);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) == (9 - i));

        pos = cx_list_next(list, pos);
        i++;
    }

    cx_list_delete(list);


    /*
     * Test 12: Create two lists from the numbers num1 splice both lists
     *          by moving all elements of the second list to the first,
     *          inserting the range of elements before the first element
     *          of the first list.
     */

    list = cx_list_new();
    _list = cx_list_new();

    for (i = 0; i < 5; i++)
        cx_list_push_back(list, &num1[i]);

    for (i = 5; i < 10; i++)
        cx_list_push_back(_list, &num1[i]);


    /* Target position */

    pos = cx_list_begin(list);

    /* Range to move */

    first = cx_list_begin(_list);
    last = cx_list_end(_list);

    cx_list_splice(list, pos, _list, first, last);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cxint val = i < 5 ? (i + 5) : (i - 5);

        cx_assert(*((cxint *)data) == val);

        pos = cx_list_next(list, pos);
        i++;
    }

    cx_assert(cx_list_empty(_list));

    cx_list_delete(list);
    cx_list_delete(_list);


    /*
     * Test 13: Create two lists from the numbers num1 splice both lists
     *          by moving all elements of the second list to the first
     *          inserting the range of elements before the end of the
     *          first list.
     */

    list = cx_list_new();
    _list = cx_list_new();

    for (i = 0; i < 5; i++)
        cx_list_push_back(_list, &num1[i]);

    for (i = 5; i < 10; i++)
        cx_list_push_back(list, &num1[i]);


    /* Target position */

    pos = cx_list_end(list);

    /* Range to move */

    first = cx_list_begin(_list);
    last = cx_list_end(_list);

    cx_list_splice(list, pos, _list, first, last);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cxint val = i < 5 ? (i + 5) : (i - 5);

        cx_assert(*((cxint *)data) == val);

        pos = cx_list_next(list, pos);
        i++;
    }

    cx_assert(cx_list_empty(_list));

    cx_list_delete(list);
    cx_list_delete(_list);


    /*
     * Test 14: Create a list from the numbers num1 move the tail of the
     *          list immediately in front of the first element using
     *          the splice function with source and target list being
     *          identical.
     */

    list = cx_list_new();

    for (i = 0; i < 10; i++)
        cx_list_push_back(list, &num1[i]);

    /* Target position */

    pos = cx_list_begin(list);

    /* Range to move */

    first = cx_list_begin(list);

    for (i = 0; i < 5; i++)
        first = cx_list_next(list, first);

    last = cx_list_end(list);

    cx_list_splice(list, pos, list, first, last);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cxint val = i < 5 ? (i + 5) : (i - 5);

        cx_assert(*((cxint *)data) == val);

        pos = cx_list_next(list, pos);
        i++;
    }

    cx_list_delete(list);


    /*
     * Test 15: Create two sorted lists from the numbers num1 and merge
     *          them into a single list. Verify the data.
     */

    list = cx_list_new();
    _list = cx_list_new();

    for (i = 0; i < 5; i++) {
        cx_list_push_back(list, &num1[2*i]);
        cx_list_push_back(_list, &num1[2*i + 1]);
    }

    cx_list_merge(list, _list, cx_test_list_compare1);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) == i);

        pos = cx_list_next(list, pos);
        i++;
    }

    cx_assert(cx_list_empty(_list));

    cx_list_delete(list);
    cx_list_delete(_list);


    /*
     * Test 16: Create a list from the numbers num2 and sort it using both
     *          comparison functions. Verify the data.
     */

    list = cx_list_new();

    for (i = 0; i < 10; i++)
        cx_list_push_back(list, &num2[i]);

    cx_list_sort(list, cx_test_list_compare1);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) == i);

        pos = cx_list_next(list, pos);
        i++;
    }

    cx_list_clear(list);

    for (i = 0; i < 10; i++)
        cx_list_push_back(list, &num2[i]);

    cx_list_sort(list, cx_test_list_compare2);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) == (9 - i));

        pos = cx_list_next(list, pos);
        i++;
    }

    cx_list_delete(list);


    /*
     * Test 17: Create two lists from the numbers num1 and num2. Swap
     *          thei contents and verify the data.
     */

    list = cx_list_new();
    _list = cx_list_new();

    for (i = 0; i < 10; i++)
        cx_list_push_back(list, &num1[i]);

    for (i = 0; i < 10; i++)
        cx_list_push_back(_list, &num2[i]);

    cx_list_swap(list, _list);

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cx_assert(*((cxint *)data) == num2[i]);

        pos = cx_list_next(list, pos);
        i++;
    }

    i = 0;
    pos = cx_list_begin(_list);

    while (pos != cx_list_end(_list)) {
        cxptr data = cx_list_get(_list, pos);
        cx_assert(*((cxint *)data) == num1[i]);

        pos = cx_list_next(_list, pos);
        i++;
    }

    cx_list_delete(list);
    cx_list_delete(_list);


    /*
     * Test 18: Create a list from allocated strings. Erase an element from
     *          the list destroying the stored data too. Verify the data.
     */

    list = cx_list_new();

    for (i = 0; (cxsize)i < strlen(letters); i++) {
        cxchar *s = cx_malloc(4 * sizeof(cxchar));
        memset(s, letters[i], 3);
        s[3] = '\0';

        cx_list_push_back(list, s);
    }

    pos = cx_list_begin(list);
    for (i = 0; i < 5; i++)
        pos = cx_list_next(list, pos);

    cx_list_erase(list, pos, cx_free);

    cx_assert(cx_list_size(list) == (strlen(letters) - 1));

    i = 0;
    pos = cx_list_begin(list);

    while (pos != cx_list_end(list)) {
        cxptr data = cx_list_get(list, pos);
        cxchar c = i < 5 ? letters[i] : letters[i + 1];

        cx_assert(*((cxchar *)data) == c);

        pos = cx_list_next(list, pos);
        i++;
    }

    cx_list_destroy(list, cx_free);

    /*
     * All tests succeeded
     */

    return 0;

}
