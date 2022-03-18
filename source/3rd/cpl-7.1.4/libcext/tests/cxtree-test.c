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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#undef CX_DISABLE_ASSERT
#undef CX_LOG_DOMAIN

/*
 * FIXME: Needed to get sigaction when C standard is set to strict c99.
 *        A more portable way of asynchronously checking for a timeout
 *        would be preferred!
 */

#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200112L
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "cxmemory.h"
#include "cxmessages.h"
#include "cxtree.h"



/*
 * The name of the current test being run. This should be updated just before
 * calling the alarm() routine in a test for infinite loops so we can print
 * a more informative message in the signal handler.
 */

static const cxchar* current_test_name = "unknown";


/*
 * Signal handler for the alarm timer to prevent the infinit loop test from
 * actually hanging forever.
 */

static void
cx_signal_handler(cxint signo)
{

    if (signo == SIGALRM) {

        /*
         * Should normally be very careful about what we call inside an
         * asynchronous signal handler. But if we arrived at this point then
         * we assume we have an infinit loop, which is an error condition.
         * Anyway cx_error() will abort the process, so we dont care so much.
         */

        cx_error("An infinite loop detected in: %s.", current_test_name);

    }

	return;

}


static cxbool
cx_test_tree_less_char(cxcptr a, cxcptr b)
{

    cxchar _a = *((const cxchar *)a);
    cxchar _b = *((const cxchar *)b);

    return _a - _b < 0 ? TRUE : FALSE;

}


static cxbool
cx_test_tree_greater_char(cxcptr a, cxcptr b)
{

    cxchar _a = *((const cxchar *)a);
    cxchar _b = *((const cxchar *)b);

    return _a - _b > 0 ? TRUE : FALSE;

}


#ifdef CX_TEST_TREE_DUMP
static void
cx_test_tree_dump(cx_tree *tree, cxint fmt)
{

    cxint i;
    cx_tree_iterator pos;


    fprintf(stderr, "tree at address %p:\n", tree);

    i = 0;
    pos = cx_tree_begin(tree);

    while (pos != cx_tree_end(tree)) {
        fprintf(stderr, "tree node %d at address %p has value ", i + 1,
                pos);

        switch (fmt) {
            case 0:
                fprintf(stderr, "%s", (cxchar *)cx_tree_get_value(tree, pos));
                break;

            case 1:
                fprintf(stderr, "%c",
                        *((cxint *)cx_tree_get_value(tree, pos)));
                break;

            default:
                fprintf(stderr, "at address %p of unknown type",
                        cx_tree_get_value(tree, pos));
                break;
        }

        fprintf(stderr, "\n");

        pos = cx_tree_next(tree, pos);
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

    cxsize i, j;

    cx_tree *tree, *_tree;
    cx_tree_iterator pos, _pos;
    cx_tree_iterator first, last;

    struct sigaction signal_info;


    /*
     * Register the signal handler for the alarm signal which is used during
     * the infinite loop tests.
     */

    memset(&signal_info, 0x0, sizeof signal_info);

    signal_info.sa_handler = &cx_signal_handler;
    sigemptyset(&signal_info.sa_mask);

    if (sigaction(SIGALRM, &signal_info, NULL) != 0) {
        cx_error("Could not set signal handler: %s", strerror(errno));
        return EXIT_FAILURE;
    }


    /*
     * Test 1: Create a tree, check that it is a valid empty tree, do
     *         some consistency checks and destroy it again.
     */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);
    cx_assert(tree != NULL);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(cx_tree_empty(tree));
    cx_assert(cx_tree_size(tree) == 0);
    cx_assert(cx_tree_begin(tree) == cx_tree_end(tree));
    cx_assert(cx_tree_begin(tree) == cx_tree_next(tree, cx_tree_end(tree)));
    cx_assert(cx_tree_begin(tree) ==
              cx_tree_previous(tree, cx_tree_end(tree)));
    cx_assert(cx_test_tree_less_char == cx_tree_key_comp(tree));

    cx_tree_delete(tree);


    /*
     * Test 2: Create two trees and fill them with the characters letters[]
     *         using both insertion methods. Check that the tree structures
     *         are valid, verify the data of both trees and compare the
     *         contents of both trees.
     */

    /* insert unique */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);

    for (i = 0; i < strlen(letters); i++)
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);

    cx_assert(cx_tree_verify(tree) != FALSE);

    i = 0;
    pos = cx_tree_begin(tree);

    while (pos != cx_tree_end(tree)) {
        cxptr data = cx_tree_get_value(tree, pos);
        cx_assert(*((cxchar *)data) == letters[i]);

        pos = cx_tree_next(tree, pos);
        i++;
    }


    /* insert equal */

    _tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);

    for (i = 0; i < strlen(letters); i++)
        cx_tree_insert_equal(_tree, &letters[i], &letters[i]);

    cx_assert(cx_tree_verify(_tree) != FALSE);

    i = 0;
    pos = cx_tree_begin(_tree);

    while (pos != cx_tree_end(_tree)) {
        cxptr data = cx_tree_get_value(_tree, pos);
        cx_assert(*((cxchar *)data) == letters[i]);

        pos = cx_tree_next(_tree, pos);
        i++;
    }

    /* Compare data of both trees */

    i = 0;
    pos = cx_tree_begin(tree);
    _pos = cx_tree_begin(_tree);

    while (pos != cx_tree_end(tree)) {
        cxptr data;

        data = cx_tree_get_key(tree, pos);
        cx_assert(*((cxchar *)data) == letters[i]);

        data = cx_tree_get_value(tree, pos);
        cx_assert(*((cxchar *)data) == letters[i]);

        data = cx_tree_get_key(_tree, _pos);
        cx_assert(*((cxchar *)data) == letters[i]);

        data = cx_tree_get_value(_tree, _pos);
        cx_assert(*((cxchar *)data) == letters[i]);

        pos = cx_tree_next(tree, pos);
        _pos = cx_tree_next(_tree, _pos);
        i++;
    }

    cx_tree_delete(tree);
    cx_tree_delete(_tree);


    /*
     * Test 3: Create a tree and fill it with the characters letters[],
     *         verify that we can walk through the tree forwards and
     *         backwards.
     */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);

    for (i = 0; i < strlen(letters); i++)
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);

    /* forwards */
    pos = cx_tree_begin(tree);

    for (i = 0; i < strlen(letters); i++)
        pos = cx_tree_next(tree, pos);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(pos == cx_tree_end(tree));


    /* backwards */

    pos = cx_tree_end(tree);

    for (i = 0; i < strlen(letters); i++)
        pos = cx_tree_previous(tree, pos);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(pos == cx_tree_begin(tree));

    cx_tree_delete(tree);


    /*
     * Test 4: Create a tree and fill it with the characters letters[],
     *         remove some elements and verify the tree structure.
     */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);

    for (i = 0; i < strlen(letters); i++)
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);

    for (i = 0; i < 10; i++)
        cx_tree_erase(tree, &letters[i]);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(cx_tree_size(tree) == strlen(letters) - 10);

    cx_tree_delete(tree);


    /*
     * Test 5: Create a tree and fill it with the characters letters[],
     *         erase nodes by position and verify the tree structure.
     */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);

    for (i = 0; i < strlen(letters); i++)
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);

    first = cx_tree_begin(tree);
    for (i = 0; i < 10; i++)
        first = cx_tree_next(tree, first);

    for (i = 10; i < 36; i++) {
        pos = cx_tree_next(tree, first);
        cx_tree_erase_position(tree, first);
        first = pos;

        cx_assert(cx_tree_verify(tree) != FALSE);
    }

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert((cx_tree_size(tree) == strlen(letters) - 26));

    cx_tree_delete(tree);


    /*
     * Test 6: Create a tree and fill it with the characters letters[],
     *         erase a range of nodes and verify the tree structure.
     */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);

    for (i = 0; i < strlen(letters); i++)
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);

    first = cx_tree_begin(tree);
    for (i = 0; i < 10; i++)
        first = cx_tree_next(tree, first);

    last = first;
    for (i = 10; i < 36; i++)
        last = cx_tree_next(tree, last);

    cx_tree_erase_range(tree, first, last);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert((cx_tree_size(tree) == strlen(letters) - 26));

    cx_tree_delete(tree);


    /*
     * Test 7: Create a tree and fill it with the characters letters[] and
     *         clear it again. Verify the tree structure and check that the
     *         tree is empty.
     */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);

    for (i = 0; i < strlen(letters); i++)
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);

    cx_tree_clear(tree);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(cx_tree_empty(tree));
    cx_assert(cx_tree_size(tree) == 0);

    cx_tree_delete(tree);


    /*
     * Test 7: Create a tree and fill it with characters from letters[] and
     *         verify that the size of the tree is reported correctly.
     */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);

    for (i = 0; i < 5; i++)
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);

    cx_assert(cx_tree_size(tree) == 5);

    for (i = 5; i < 26; i++)
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);

    cx_assert(cx_tree_size(tree) == 5 + 21);

    for (i = 26; i < strlen(letters); i++)
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);

    cx_assert(cx_tree_size(tree) == strlen(letters));

    cx_tree_delete(tree);


    /*
     * Test 8: Create a tree and fill it with characters from letters[],
     *         try to locate a particular value by looking for its key.
     *         Verify that the entry found is the correct one.
     */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);

    for (i = 0; i < strlen(letters); i++)
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);

    pos = cx_tree_find(tree, &letters[25]);

    cx_assert(pos != cx_tree_end(tree));
    cx_assert(*((cxchar *)cx_tree_get_key(tree, pos)) == 'P');
    cx_assert(*((cxchar *)cx_tree_get_value(tree, pos)) == 'P');


    /*
     * Test 9: Assign a new value to the tree node from the previous test
     *         and verify the data and the tree structure.
     */

    cx_tree_assign(tree, pos, &letters[23]);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(*((cxchar *)cx_tree_get_key(tree, pos)) == 'P');
    cx_assert(*((cxchar *)cx_tree_get_value(tree, pos)) == 'N');

    cx_tree_assign(tree, pos, &letters[25]);


    /*
     * Test 10: Count the occurrences of a prticular key in the tree from
     *          the previous test. For this tree the result should be 1
     *          for any key.
     */

    for (i = 0; i < strlen(letters); i++)
        cx_assert(cx_tree_count(tree, &letters[i]) == 1);


    /*
     * Test 10: Using the tree from the previous test check the lower
     *          and upper bounds for a particular key. With this tree
     *          the upper bound should always be the successor of the
     *          lower bound.
     */

    for (i = 0; i < strlen(letters); i++) {
        pos = cx_tree_lower_bound(tree, &letters[i]);
        _pos = cx_tree_upper_bound(tree, &letters[i]);

        cx_assert(*((cxchar *)cx_tree_get_value(tree, pos)) == letters[i]);
        cx_assert(_pos == cx_tree_end(tree) ||
                  *((cxchar *)cx_tree_get_value(tree, _pos)) ==
                  letters[i + 1]);
        cx_assert(_pos == cx_tree_next(tree, pos));
    }

    pos = cx_tree_lower_bound(tree, &letters[25]);
    _pos = cx_tree_upper_bound(tree, &letters[25]);

    cx_tree_equal_range(tree, &letters[25], &first, &last);

    cx_assert(first == pos);
    cx_assert(last == _pos);

    cx_tree_delete(tree);


    /*
     * Test 11: Create two trees, an empty one and one filled with the
     *          characters letters[]. Swap the contents of the two trees
     *          and validate the tree structure and the data.
     */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);
    _tree = cx_tree_new(cx_test_tree_greater_char, NULL, NULL);

    for (i = 0; i < strlen(letters); i++) {
        cx_tree_insert_unique(tree, &letters[i], &letters[i]);
        cx_tree_insert_unique(_tree, &letters[i], &letters[i]);
    }

    cx_tree_swap(tree, _tree);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(cx_tree_verify(_tree) != FALSE);

    cx_assert(cx_tree_key_comp(tree) == cx_test_tree_greater_char);
    cx_assert(cx_tree_key_comp(_tree) == cx_test_tree_less_char);

    i = 0;
    pos = cx_tree_begin(tree);

    while (pos != cx_tree_end(tree)) {
        cxptr data = cx_tree_get_value(tree, pos);
        cx_assert(*((cxchar *)data) == letters[strlen(letters) - i - 1]);

        pos = cx_tree_next(tree, pos);
        i++;
    }

    i = 0;
    pos = cx_tree_begin(_tree);

    while (pos != cx_tree_end(_tree)) {
        cxptr data = cx_tree_get_value(_tree, pos);
        cx_assert(*((cxchar *)data) == letters[i]);

        pos = cx_tree_next(_tree, pos);
        i++;
    }

    cx_tree_delete(tree);
    cx_tree_delete(_tree);


    /*
     * Redo some of the tests above for trees with multiple occurrence
     * of the same key.
     */

    /*
     * Test 12: Create a tree and insert the characters letters[] several
     *          times
     */

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);

    for (j = 0; j < 5; j++)
        for (i = 0; i < strlen(letters); i++)
            cx_tree_insert_equal(tree, &letters[i], &letters[i]);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(cx_tree_size(tree) == (5 * strlen(letters)));

    i = 0;
    pos = cx_tree_begin(tree);

    while (pos != cx_tree_end(tree)) {
        cxptr data = cx_tree_get_value(tree, pos);
        cx_assert(*((cxchar *)data) == letters[i / 5]);

        pos = cx_tree_next(tree, pos);
        i++;
    }


    /*
     * Test 13: Use the tree from the previous test to verify that keys
     *          are properly looked up.
     */

    cx_assert(cx_tree_lower_bound(tree, &letters[25]) ==
              cx_tree_find(tree, &letters[25]));

    for (i = 0; i < strlen(letters); i++)
        cx_assert(cx_tree_count(tree, &letters[i]) == 5);

    pos = cx_tree_lower_bound(tree, &letters[25]);
    for (i = 0; i < 5; i++)
        pos = cx_tree_next(tree, pos);

    cx_assert(pos == cx_tree_upper_bound(tree, &letters[25]));

    cx_tree_equal_range(tree, &letters[25], &first, &last);
    cx_assert(first == cx_tree_lower_bound(tree, &letters[25]));
    cx_assert(last == cx_tree_upper_bound(tree, &letters[25]));


    /*
     * Test 14: Use the tree from the previous test to verify that element
     *          removal works properly.
     */

    pos = cx_tree_lower_bound(tree, &letters[25]);
    cx_tree_erase_position(tree, pos);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(cx_tree_size(tree) == (5 * strlen(letters) - 1));
    cx_assert(cx_tree_count(tree, &letters[25]) == 4);


    /*
     * Test 14: Use the tree from the previous test to verify that removing
     *          a range of elements works.
     */

    first = cx_tree_lower_bound(tree, &letters[23]);
    last = cx_tree_upper_bound(tree, &letters[25]);

    cx_tree_erase_range(tree, first, last);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(cx_tree_size(tree) == (5 * (strlen(letters) - 3)));
    cx_assert(cx_tree_count(tree, &letters[23]) == 0);
    cx_assert(cx_tree_find(tree, &letters[23]) == cx_tree_end(tree));
    cx_assert(cx_tree_lower_bound(tree, &letters[25]) ==
              cx_tree_upper_bound(tree, &letters[25]));


    /*
     * Test 14: Use the tree from the previous test to verify that removing
     *          all elements with a particular key works.
     */


    cx_assert(cx_tree_erase(tree, &letters[27]) == 5);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(cx_tree_size(tree) == (5 * (strlen(letters) - 4)));
    cx_assert(cx_tree_count(tree, &letters[27]) == 0);
    cx_assert(cx_tree_find(tree, &letters[27]) == cx_tree_end(tree));
    cx_assert(cx_tree_lower_bound(tree, &letters[27]) ==
              cx_tree_upper_bound(tree, &letters[27]));


    /*
     * Test 15: Use the tree from the previous test to verify that removing
     *          all elements works.
     */

    cx_tree_clear(tree);

    cx_assert(cx_tree_verify(tree) != FALSE);
    cx_assert(cx_tree_empty(tree));
    cx_assert(cx_tree_size(tree) == 0);

    cx_tree_delete(tree);


    /*
     * Test 16: The following code sequence causes an infinite loop at
     *          cxtree.c:447 for broken versions of libcext 0.1.0.
     *          Should check if this still happens for new versions.
     */

    current_test_name = "Test 16 for infinite loop in cx_tree_erase_position";

    /*
     * Set the alarm for 10 seconds. If this test is not done by then we have
     * to assume we are in an infinite loop.
     */

    alarm(10);

    tree = cx_tree_new(cx_test_tree_less_char, NULL, NULL);
    cx_assert(tree != NULL);
    cx_tree_erase_position(tree, cx_tree_begin(tree));
    cx_tree_delete(tree);

	/*
     *  Will stop any more alarm signals for subsequent tests.
     */

    alarm(0);


    /*
     * All tests succeeded
     */

    return 0;

}
