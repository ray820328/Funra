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
#include "cxdeque.h"


/*
 * Compares letters as small integer numbers
 */

static cxint
_cx_test_compare_letters(cxcptr first, cxcptr second)
{

    cxint _first  = *(cxchar *)first;
    cxint _second = *(cxchar *)second;

    return _first - _second;

}


static cxint
_cx_test_compare_numbers(cxcptr first, cxcptr second)
{

    cxint _first  = *(const cxint *)first;
    cxint _second = *(const cxint *)second;

    return _first - _second;

}

#ifdef CX_TEST_DEQUE_DUMP
static void
_cx_test_deque_dump(cx_deque *deque, cxint fmt)
{

    cxsize i;
    cx_deque_iterator pos;


    fprintf(stderr, "deque at start address %p:\n", deque);

    i = 0;
    pos = cx_deque_begin(deque);

    while (pos != cx_deque_end(deque)) {
        fprintf(stderr, "deque element %lu at position %lu has value ", i + 1,
                pos);

        switch (fmt) {
            case 0:
                fprintf(stderr, "%s", (cxchar *)cx_deque_get(deque, pos));
                break;

            case 1:
                fprintf(stderr, "%d", *((cxint *)cx_deque_get(deque, pos)));
                break;

            default:
                fprintf(stderr, "of unknown type at address %p",
                        cx_deque_get(deque, pos));
                break;
        }

        fprintf(stderr, "\n");

        pos = cx_deque_next(deque, pos);
        ++i;
    }

    return;

}
#endif


int
main(void)
{

    cxchar letters[]  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    cxchar unsorted[] = "ZYXWVUTSRQPONMLKJIHGFEDCBA";

    cxint ival = 0;
    cxint num1[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    cxint num2[10] = {8, 9, 7, 0, 3, 2, 5, 1, 4, 6};

    cxsize i  = 0;
    cxsize sz = 0;

    cx_deque *d1 = NULL;
    cx_deque *d2 = NULL;

    cx_deque_iterator pos;
    cx_deque_iterator next;


    /*
     * Test 1: Create a deque, check that it is a valid empty deque and
     *         destroy it again.
     */

    d1 = cx_deque_new();
    cx_assert(d1 != NULL);

    cx_assert(cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 0);
    cx_assert(cx_deque_begin(d1) == cx_deque_end(d1));

    cx_deque_delete(d1);


    /*
     * Test 2: Create a deque and append the numbers num1, check that
     *         the data were properly appended, thereby verifying that
     *         we can properly step through the array.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    cx_assert(cx_deque_size(d1) == 10);

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cx_assert(*((cxint *)data) == num1[i]);

        pos = cx_deque_next(d1, pos);
        ++i;
    }
    
    i = 0;
    pos = cx_deque_previous(d1, cx_deque_end(d1));

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cx_assert(*((cxint *)data) == num1[9 - i]);

        pos = cx_deque_previous(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);


    /*
     * Test 3: Same as test 2, but insert the numbers as head of the array.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_front(d1, &num1[i]);
    }

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cx_assert(*((cxint *)data) == num1[9 - i]);

        pos = cx_deque_next(d1, pos);
        i++;
    }

    cx_deque_destroy(d1, NULL);


    /*
     * Test 4: Check accessors to the first and the last element of a deque
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_front(d1, &num1[i]);
    }

    cx_assert(*(cxint *)cx_deque_front(d1) == num1[9]);
    cx_assert(*(cxint *)cx_deque_back(d1)  == num1[0]);

    cx_deque_destroy(d1, NULL);


    /*
     * Test 5: Create deques with different sizes using the numbers num1 as
     *         elements and check that the size of the deque is properly
     *         reported.
     */

    /* 10 elements */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 10);

    cx_deque_clear(d1);


    /* 5 elements */

    for (i = 0; i < 5; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 5);

    cx_deque_clear(d1);


    /* 1 element */

    for (i = 0; i < 1; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 1);

    cx_deque_destroy(d1, NULL);


    /*
     * Test 6: Swap the data of 2 deques and check the element assignment
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    d2 = cx_deque_new();

    for (i = 0; i < 5; ++i) {
        cx_deque_push_back(d2, &num2[i]);
    }

    cx_deque_swap(d1, d2);

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 5);

    cx_assert(!cx_deque_empty(d2));
    cx_assert(cx_deque_size(d2) == 10);

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cx_assert(*(cxint *)data == num2[i]);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    i = 0;
    pos = cx_deque_begin(d2);

    while (pos != cx_deque_end(d2)) {
        cxptr data = cx_deque_get(d2, pos);
        cx_assert(*(cxint *)data == num1[i]);

        pos = cx_deque_next(d2, pos);
        ++i;
    }

    cx_deque_destroy(d2, NULL);
    cx_deque_destroy(d1, NULL);


    /*
     * Test 7: Assign the number num1[4] to the 6th and 9th position of the
     *         deque. Verify the data.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    pos = cx_deque_begin(d1);
    for (i = 0; i < 5; ++i) {
        pos = cx_deque_next(d1, pos);
    }

    cx_assert(cx_deque_get(d1, pos) != &num1[4]);

    ival = *(cxint *)cx_deque_assign(d1, pos, &num1[4]);

    cx_assert(ival == 5);
    cx_assert(cx_deque_get(d1, pos) == &num1[4]);
    cx_assert(*(cxint *)cx_deque_get(d1, pos) == num1[4]);

    for (i = 5; i < 8; ++i) {
        pos = cx_deque_next(d1, pos);
    }

    cx_assert(cx_deque_get(d1, pos) != &num1[4]);

    ival = *(cxint *)cx_deque_assign(d1, pos, &num1[4]);

    cx_assert(ival == 8);
    cx_assert(cx_deque_get(d1, pos) == &num1[4]);
    cx_assert(*(cxint *)cx_deque_get(d1, pos) == num1[4]);

    cx_deque_destroy(d1, NULL);


    /*
     * Test 8: Create a deque and remove elements from the head and
     *         the tail. Check the data.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_front(d1, &num1[i]);
    }

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 10);

    ival = *((cxint *)cx_deque_pop_front(d1));

    cx_assert(ival == 9);
    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 9);

    ival = *((cxint *)cx_deque_pop_back(d1));

    cx_assert(ival == 0);
    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 8);

    i = 1;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cx_assert(*((cxint *)data) == num1[9 - i]);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);


    /*
     * Test 9: Remove all elements from a deque and verify that the
     *         deque is empty.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    cx_assert(!cx_deque_empty(d1));

    cx_deque_clear(d1);

    cx_assert(cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 0);

    cx_deque_delete(d1);


    /*
     * Test 10: Create a deque from allocated strings. Erase the element
     *          at the 6th position from the deque and verify the data.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cxchar *s = cx_malloc(4 * sizeof(cxchar));
        memset(s, letters[i], 3);
        s[3] = '\0';

        cx_deque_push_back(d1, s);
    }

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 10);

    pos = cx_deque_begin(d1);
    for (i = 0; i < 5; ++i) {
        pos = cx_deque_next(d1, pos);
    }

    next = cx_deque_erase(d1, pos, cx_free);

    cx_assert(next == pos);
    cx_assert(cx_deque_size(d1) == 9);

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cxchar c = i < 5 ? letters[i] : letters[i + 1];

        cx_assert(*((cxchar *)data) == c);

        pos =cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, cx_free);


    /*
     * Test 11: Create a deque from the numbers num1. Insert the
     *          number num1[5] as the 6th deque element and verify
     *          the data.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 10);

    next = cx_deque_begin(d1);

    for (i = 0; i < 5; ++i) {
        next = cx_deque_next(d1, next);
    }

    pos = cx_deque_insert(d1, next, &num1[5]);

    cx_assert(pos == next);
    cx_assert(cx_deque_size(d1) == 11);
    cx_assert(*((cxint *)cx_deque_get(d1, pos)) == 5);

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cx_assert(*((cxint *)data) == num1[i]);

        if (pos != next) {
            ++i;
        }

        pos = cx_deque_next(d1, pos);

    }

    cx_deque_destroy(d1, NULL);


    /*
     * Test 12: Create a deque from the numbers num1. Extract the element
     *          at the 6th position from the deque and verify the data.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 10);

    pos = cx_deque_begin(d1);
    for (i = 0; i < 5; ++i) {
        pos = cx_deque_next(d1, pos);
    }

    ival = *((cxint *)cx_deque_extract(d1, pos));

    cx_assert(ival == 5);
    cx_assert(cx_deque_size(d1) == 9);

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);

        if (i == 5) {
            ++i;
        }

        cx_assert(*(cxint *)data == num1[i]);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);


    /*
     * Test 13: Create a deque from the numbers num1, with multiple
     *          occurrences of num[4]. Remove all deque elements with value
     *          num1[4] from the deque. Verify the data.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    cx_deque_insert(d1, cx_deque_begin(d1) + 5, &num1[4]);
    cx_deque_insert(d1, cx_deque_begin(d1) + 8, &num1[4]);

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 12);

    cx_deque_remove(d1, &num1[4]);

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 9);

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cx_assert(data != &num1[4]);
        cx_assert(*(cxint *)data != num1[4]);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);


    /*
     * Test 14: Create a deque from the numbers num1 with a duplicates
     *          immediately following each other at position 6. Remove
     *          consecutive duplicates from the deque. Verify the data.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    cx_deque_insert(d1, cx_deque_begin(d1) + 5, &num1[4]);
    cx_deque_insert(d1, cx_deque_begin(d1) + 5, &num1[4]);

    cx_assert(cx_deque_size(d1) == 12);
    cx_assert(*(cxint *)cx_deque_get(d1, cx_deque_begin(d1) + 4) == num1[4]);
    cx_assert(*(cxint *)cx_deque_get(d1, cx_deque_begin(d1) + 5) == num1[4]);
    cx_assert(*(cxint *)cx_deque_get(d1, cx_deque_begin(d1) + 6) == num1[4]);

    cx_deque_unique(d1, _cx_test_compare_numbers);

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 10);

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cx_assert(*(cxint *)data == num1[i]);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);


    /*
     * Test 15: Splice two deques together. Check this for various
     *          combinations of source and target positions.
     *
     */

    sz = CX_N_ELEMENTS(unsorted) - 1;


    /* beginning of source to end of target */

    d1 = cx_deque_new();

    for (i = 0; i < sz; ++i) {
        cx_deque_push_back(d1, &unsorted[i]);
    }

    d2 = cx_deque_new();
    cx_assert(cx_deque_empty(d2));

    cx_deque_splice(d2, cx_deque_end(d2), d1, cx_deque_begin(d1),
                    cx_deque_begin(d1) + 5);

    cx_assert(cx_deque_size(d2) == 5);
    cx_assert(cx_deque_size(d1) == sz - 5);

    pos = cx_deque_begin(d2);

    for (i = 0; i < 5; ++i) {

        cxptr data = cx_deque_get(d2, pos);

        cx_assert(*((cxchar *)data) == unsorted[i]);
        pos = cx_deque_next(d2, pos);

    }

    pos = cx_deque_begin(d1);

    for (i = 5; i < sz; ++i) {

        cxptr data = cx_deque_get(d1, pos);

        cx_assert(*((cxchar *)data) == unsorted[i]);
        pos = cx_deque_next(d1, pos);

    }

    cx_deque_destroy(d2, NULL);
    cx_deque_destroy(d1, NULL);


    /* end of source to end of target */

    d1 = cx_deque_new();

    for (i = 0; i < sz; ++i) {
        cx_deque_push_back(d1, &unsorted[i]);
    }

    d2 = cx_deque_new();
    cx_assert(cx_deque_empty(d2));

    cx_deque_splice(d2, cx_deque_end(d2), d1, cx_deque_end(d1) - 5,
                    cx_deque_end(d1));

    cx_assert(cx_deque_size(d2) == 5);
    cx_assert(cx_deque_size(d1) == sz - 5);

    pos = cx_deque_begin(d2);

    for (i = 0; i < 5; ++i) {

        cxsize k = sz - 5 + i;
        cxptr data = cx_deque_get(d2, pos);

        cx_assert(*((cxchar *)data) == unsorted[k]);
        pos = cx_deque_next(d2, pos);

    }

    pos = cx_deque_begin(d1);

    for (i = 0; i < sz - 5; ++i) {

        cxptr data = cx_deque_get(d1, pos);

        cx_assert(*((cxchar *)data) == unsorted[i]);
        pos = cx_deque_next(d1, pos);

    }

    cx_deque_destroy(d2, NULL);
    cx_deque_destroy(d1, NULL);


    /* middle of source to middle of target */

    d1 = cx_deque_new();

    for (i = 0; i < sz; ++i) {
        cx_deque_push_back(d1, &unsorted[i]);
    }

    d2 = cx_deque_new();

    for (i = 0; i < sz; ++i) {
        cx_deque_push_back(d2, &letters[i]);
    }

    cx_assert(cx_deque_size(d2) == sz);

    cx_deque_splice(d2, cx_deque_begin(d2) + 5, d1,
                    cx_deque_begin(d1) + 5, cx_deque_begin(d1) + 10);

    cx_assert(cx_deque_size(d2) == sz + 5);
    cx_assert(cx_deque_size(d1) == sz - 5);

    pos = cx_deque_begin(d2);

    for (i = 0; i < 5; ++i) {

        cxptr data = cx_deque_get(d2, pos);

        cx_assert(*((cxchar *)data) == letters[i]);
        pos = cx_deque_next(d2, pos);

    }

    for (i = 5; i < 10; ++i) {

        cxptr data = cx_deque_get(d2, pos);

        cx_assert(*((cxchar *)data) == unsorted[i]);
        pos = cx_deque_next(d2, pos);

    }

    for (i = 5; i < sz; ++i) {

        cxptr data = cx_deque_get(d2, pos);

        cx_assert(*((cxchar *)data) == letters[i]);
        pos = cx_deque_next(d2, pos);

    }

    pos = cx_deque_begin(d1);

    for (i = 0; i < 5; ++i) {

        cxptr data = cx_deque_get(d1, pos);

        cx_assert(*((cxchar *)data) == unsorted[i]);
        pos = cx_deque_next(d1, pos);

    }

    for (i = 10; i < sz; ++i) {

        cxptr data = cx_deque_get(d1, pos);

        cx_assert(*((cxchar *)data) == unsorted[i]);
        pos = cx_deque_next(d1, pos);

    }

    cx_deque_destroy(d2, NULL);
    cx_deque_destroy(d1, NULL);


    /*
     * Test 16: Create a deque from the numbers num1. Test the moving of a
     *          range of deque nodes using the splice function for several
     *          use cases in case the source and the target deque are identical.
     *  Move the tail of the
     *          deque immediately in front of the first element using
     *          the splice function with source and target deque being
     *          identical.
     */

    /* Move the tail of the deque immediately in front of the first element */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    pos = cx_deque_begin(d1) + 5;

    cx_deque_splice(d1, cx_deque_begin(d1), d1, pos, cx_deque_end(d1));

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 10);

#ifdef CX_TEST_DEQUE_DUMP
    _cx_test_deque_dump(d1, 1);
#endif

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cxint val = i < 5 ? num1[i + 5] : num1[i - 5];

        cx_assert(*((cxint *)data) == val);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);


    /* Move the head of the deque to the end */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    pos = cx_deque_begin(d1) + 5;

    cx_deque_splice(d1, cx_deque_end(d1), d1, cx_deque_begin(d1), pos);

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 10);

#ifdef CX_TEST_DEQUE_DUMP
    _cx_test_deque_dump(d1, 1);
#endif

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cxint val = i < 5 ? num1[i + 5] : num1[i - 5];

        cx_assert(*((cxint *)data) == val);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);


    /* Move a range of nodes following the insertion position. */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    pos = cx_deque_begin(d1) + 2;

    cx_deque_splice(d1, pos, d1, cx_deque_end(d1) - 5, cx_deque_end(d1) - 2);

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 10);

#ifdef CX_TEST_DEQUE_DUMP
    _cx_test_deque_dump(d1, 1);
#endif

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cxint val = num1[i];

        if ((i >= 2) && (i < 8)) {
            val = (i < 5) ? num1[i + 3] : num1[i - 3];
        }

        cx_assert(*((cxint *)data) == val);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);


    /* Move a range of nodes preceding the insertion position. */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    pos = cx_deque_end(d1) - 5;

    cx_deque_splice(d1, cx_deque_begin(d1) + 2, d1, pos, pos + 3);

    cx_assert(!cx_deque_empty(d1));
    cx_assert(cx_deque_size(d1) == 10);

#ifdef CX_TEST_DEQUE_DUMP
    _cx_test_deque_dump(d1, 1);
#endif

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cxint val = num1[i];

        if ((i >= 2) && (i < 8)) {
            val = (i < 5) ? num1[i + 3] : num1[i - 3];
        }

        cx_assert(*((cxint *)data) == val);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);


    /*
     * Test 17: Create two sorted deques from the numbers num1 and merge
     *          them into a single deque. Verify the data.
     */

    d1 = cx_deque_new();
    d2 = cx_deque_new();

    for (i = 0; i < 5; ++i) {
        cx_deque_push_back(d1, &num1[2 * i]);
        cx_deque_push_back(d2, &num1[2 * i + 1]);
    }

    cx_deque_merge(d1, d2, _cx_test_compare_numbers);

    cx_assert(cx_deque_size(d1) == 10);
    cx_assert(cx_deque_empty(d2));


    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cx_assert(*((cxint *)data) == num1[i]);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);
    cx_deque_destroy(d2, NULL);


    /*
     * Test 18: Verify that the sorted version of unsorted[] is
     *          equal to letters[]
     */

    d1 = cx_deque_new();


    /* Ignore the trailing '\0' of the string literal */

    sz = CX_N_ELEMENTS(unsorted) - 1;


    for (i = 0; i < sz; ++i) {
        cx_deque_push_back(d1, &unsorted[i]);
    }

    cx_assert(cx_deque_size(d1) == sz);

    cx_deque_sort(d1, _cx_test_compare_letters);

    cx_assert(cx_deque_size(d1) == sz);

    i = 0;
    pos = cx_deque_begin(d1);
    
    while (pos != cx_deque_end(d1)) {

        cxptr data = cx_deque_get(d1, pos);

#ifdef CX_TEST_VERBOSE
        fprintf(stderr, "deque: %c, letters[%lu]: %c\n",
                    *(cxchar *)data, i, letters[i]);
#endif

        cx_assert((*(cxchar *)data) == letters[i]);

        pos = cx_deque_next(d1, pos);
        ++i;

    }

    cx_deque_destroy(d1, NULL);


    /*
     * Test 19: Create a deque from the numbers num1, reverse the deque and
     *          check the data.
     */

    d1 = cx_deque_new();

    for (i = 0; i < 10; ++i) {
        cx_deque_push_back(d1, &num1[i]);
    }

    cx_assert(cx_deque_size(d1) == 10);

    cx_deque_reverse(d1);

    cx_assert(cx_deque_size(d1) == 10);

    i = 0;
    pos = cx_deque_begin(d1);

    while (pos != cx_deque_end(d1)) {
        cxptr data = cx_deque_get(d1, pos);
        cx_assert(*((cxint *)data) == num1[9 - i]);

        pos = cx_deque_next(d1, pos);
        ++i;
    }

    cx_deque_destroy(d1, NULL);


    /*
     * All tests succeeded
     */

    return 0;

}
