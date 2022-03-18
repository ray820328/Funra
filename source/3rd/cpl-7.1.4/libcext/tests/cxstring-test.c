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

#include <stdlib.h>
#include <string.h>

#include "cxmemory.h"
#include "cxmessages.h"
#include "cxstring.h"


/*
 * Switch to turn messages from the individual tests on and off.
 */

static cxbool verbose = FALSE;


static cxbool
test1(void)
{

    cx_string *tmp     = NULL;
    cxbool    f_passed = TRUE;

    tmp = cx_string_new();

    if (tmp != NULL) {
        if (verbose == TRUE) {
            cx_print("cx_string_new            : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_new            : FAILED\n");
        }
        f_passed = FALSE;
    }

    if (tmp != NULL) {

        cx_string_set(tmp, "Test single set...");

        if (strcmp(cx_string_get(tmp), "Test single set...") == 0) {
            if (verbose == TRUE) {
                cx_print("cx_string_set (SINGLE)   : PASSED\n");
            }
        }
        else {
            if (verbose == TRUE) {
                cx_print("cx_string_set (SINGLE)   : FAILED\n");
            }
            f_passed = FALSE;
        }

        cx_string_delete(tmp);
    }

    return f_passed;
}


static cxbool
test2(void)
{

    cx_string *tmp     = NULL;
    cxbool    f_passed = TRUE;

    tmp = cx_string_new();

    if (tmp == NULL) {
        return FALSE;
    }

    cx_string_set(tmp, "Test Multi Set 1");
    cx_string_set(tmp, "Test Multi Set 2");
    cx_string_set(tmp, "Test Multi Set 3");
    cx_string_set(tmp, "Test Multi Set 4");
    cx_string_set(tmp, "Test Multi Set 5");
    cx_string_set(tmp, "Test Multi Set 6");
    cx_string_set(tmp, "Test Multi Set 7");
    cx_string_set(tmp, "Test Multi Set 8");
    cx_string_set(tmp, "Test Multi Set 9");


    if (strcmp(cx_string_get(tmp), "Test Multi Set 9") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_set (MULTIPLE) : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_set (MULTIPLE) : FAILED\n");
        }
        f_passed = FALSE;
    }

    cx_string_delete(tmp);
    return f_passed;

}


static cxbool
test3(void)
{

    cx_string *tmp     = NULL;
    cxbool    f_passed = TRUE;

    tmp = cx_string_create("Test Create With Value");

    if (tmp == NULL) {
        return FALSE;
    }

    if (strcmp(cx_string_get(tmp), "Test Create With Value") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_create         : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_create         : FAILED\n");
        }
        f_passed = FALSE;
    }

    cx_string_delete(tmp);
    return f_passed;

}


static cxbool
test4(void)
{

    cx_string *tmp     = NULL;
    cxbool    f_passed = TRUE;

    tmp = cx_string_new();

    if (tmp == NULL) {
        return FALSE;
    }

    cx_string_set(tmp, "PASSED");
    if (verbose == TRUE) {
        cx_print("cx_string_get            : %s\n", cx_string_get(tmp));
    }

    if (strcmp(cx_string_get(tmp), "PASSED") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_set/get        : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_set/get        : FAILED\n");
        }
        f_passed = FALSE;
    }

    cx_string_delete(tmp);
    return f_passed;

}


static cxbool
test5(void)
{

    cx_string *tmp     = NULL;
    cxbool    f_passed = TRUE;

    tmp = cx_string_new();

    if (tmp == NULL) {
        return FALSE;
    }

    cx_string_set(tmp, "   ***AaBbCcDd12345***   ");

    /* to upper */

    cx_string_upper(tmp);

    if (strcmp(cx_string_get(tmp), "   ***AABBCCDD12345***   ") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_upper          : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_upper          : FAILED\n");
        }
        f_passed = FALSE;
    }

    /* to lower */

    cx_string_lower(tmp);

    if (strcmp(cx_string_get(tmp), "   ***aabbccdd12345***   ") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_lower          : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_lower          : FAILED\n");
        }
        f_passed = FALSE;
    }

    /* prepend */

    cx_string_prepend(tmp, "  -->");

    if (strcmp(cx_string_get(tmp), "  -->   ***aabbccdd12345***   ") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_prepend        : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_prepend        : FAILED\n");
        }
        f_passed = FALSE;
    }

    /* append */

    cx_string_append(tmp, "<- * **  ");

    if (strcmp(cx_string_get(tmp),
               "  -->   ***aabbccdd12345***   <- * **  ") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_append         : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_append         : FAILED\n");
        }
        f_passed = FALSE;
    }

    /* truncate */

    cx_string_truncate(tmp, 35);

    if (strcmp(cx_string_get(tmp), "  -->   ***aabbccdd12345***   "
               "<- * ") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_truncate       : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_truncate       : FAILED\n");
        }
        f_passed = FALSE;
    }

    /* erase */

    cx_string_erase(tmp, 19, 5);

    if (strcmp(cx_string_get(tmp), "  -->   ***aabbccdd***   <- * ") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_erase          : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_erase          : FAILED\n");
        }
        f_passed = FALSE;
    }

    /* insert */

    cx_string_insert(tmp, 19, "54321");

    if (strcmp(cx_string_get(tmp), "  -->   ***aabbccdd54321***   "
               "<- * ") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_insert         : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_insert         : FAILED\n");
        }
        f_passed = FALSE;
    }


    /* trim */

    cx_string_trim(tmp);

    if (strcmp(cx_string_get(tmp), "-->   ***aabbccdd54321***   <- * ") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_trim           : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_trim           : FAILED\n");
        }
        f_passed = FALSE;
    }

    /* rtrim */

    cx_string_rtrim(tmp);

    if (strcmp(cx_string_get(tmp), "-->   ***aabbccdd54321***   <- *") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_rtrim          : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_rtrim          : FAILED\n");
        }
        f_passed = FALSE;
    }

    /* strip */

    cx_string_strip(tmp);

    if (strcmp(cx_string_get(tmp), "-->   ***aabbccdd54321***   <- *") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_strip          : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_strip          : FAILED\n");
        }
        f_passed = FALSE;
    }

    cx_string_delete(tmp);

    return f_passed;
}


static cxbool test6(void) {

    cx_string *ac      = NULL;
    cx_string *bc      = NULL;
    cx_string *cc      = NULL;

    cxbool    f_passed = TRUE;

    ac = cx_string_new();
    bc = cx_string_new();

    if (ac == NULL) {
        return FALSE;
    }

    if (bc == NULL) {
        return FALSE;
    }

    cx_string_set(ac, "comparison");
    cx_string_set(bc, "COMPARISON");

    if (cx_string_equal(ac, bc) == FALSE) {
        if (verbose == TRUE) {
            cx_print("cx_string_equal          : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_equal          : FAILED\n");
        }
        f_passed = FALSE;
    }

    if (cx_string_compare(ac, bc) > 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_compare        : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_compare        : FAILED\n");
        }
        f_passed = FALSE;
    }

    /* copy */
    cc = cx_string_copy(bc);

    if (cx_string_equal(cc, bc) == TRUE) {
        if (verbose == TRUE) {
            cx_print("cx_string_equal          : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_equal          : FAILED\n");
        }
        f_passed = FALSE;
    }

    if (cx_string_compare(cc, bc) == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_compare        : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_compare        : FAILED\n");
        }
        f_passed = FALSE;
    }

    cx_string_delete(ac);
    cx_string_delete(bc);
    cx_string_delete(cc);

    return f_passed;
}

static cxbool
test7(void)
{

    cx_string *tmp     = NULL;
    cx_string *tmp2    = NULL;

    cxbool    f_passed = TRUE;

    tmp  = cx_string_new();
    tmp2  = cx_string_new();

    if (tmp == NULL) {
        return FALSE;
    }

    if (tmp2 == NULL) {
        return FALSE;
    }

    cx_string_set(tmp, "cx_string_print          : PASSED\n");
    if (verbose == TRUE) {
        cx_string_print(tmp);
    }

    cx_string_sprintf(tmp2, "This is a %s test %d", "second", 2);

    if (strcmp(cx_string_get(tmp2), "This is a second test 2") == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_sprintf        : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_sprintf        : FAILED\n");
        }
        f_passed = FALSE;
    }

    cx_string_delete(tmp2);
    cx_string_delete(tmp);

    return f_passed;
}


static cxbool
test8(void)
{
    cx_string *a       = NULL;
    cx_string *b       = NULL;

    cxbool    f_passed = TRUE;

    a = cx_string_create("this is a test string");
    b = cx_string_create("this is a test");

    if (cx_string_erase(a, 14, -1) != NULL) {
        if (verbose == TRUE) {
            cx_print("cx_string_erase          : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_erase          : FAILED\n");
        }
        f_passed = FALSE;
    }

    if (cx_string_compare(a, b) == 0) {
        if (verbose == TRUE) {
            cx_print("cx_string_compare        : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_compare        : FAILED\n");
        }
        f_passed = FALSE;
    }

    cx_string_delete(b);
    cx_string_delete(a);
    
    a = cx_string_new();
    
    if (cx_string_erase(a, 0, 0) != NULL) {
        if (verbose == TRUE) {
            cx_print("cx_string_erase          : PASSED\n");
        }
    }
    else {
        if (verbose == TRUE) {
            cx_print("cx_string_erase          : FAILED\n");
        }
        f_passed = FALSE;
    }
    
    cx_string_delete(a);

    return f_passed;
}


int
main(void) {

    cxbool f_passed = TRUE;

    verbose = getenv("VERBOSE") != NULL ? TRUE : FALSE;

    /* Test 1 : cx_string_new */
    f_passed = f_passed && test1();

    /* Test 2 : cx_string_set, single time */
    f_passed = f_passed && test2();

    /* Test 3 : cx_string_set, multiple times */
    f_passed = f_passed && test3();

    /* Test 4 : cx_string_create */
    f_passed = f_passed && test4();

    /* Test 5: cx_string get/set */
    f_passed = f_passed && test5();

    /* Test 6: cx_string in-place functions */
    f_passed = f_passed && test6();

    /* Test 7: cx_string with copy functions */
    f_passed = f_passed && test7();

    /* Test 8: cx_string erase function, end of string removal */
    f_passed = f_passed && test8();

    if (f_passed != TRUE) {
    return 1;
    }

    return 0;
}
