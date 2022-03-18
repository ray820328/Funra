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

#include <cpl_error.h>
#include <cpl_msg.h>
#include <cpl_test.h>
#include <cpl_memory.h>

#include <cpl_error_impl.h>
#include <cpl_errorstate.h>

#include <string.h>

/*-----------------------------------------------------------------------------
                       Private functions and their variables
 -----------------------------------------------------------------------------*/

static void my_error_counter(unsigned, unsigned, unsigned);
static unsigned nerrors;
static cpl_boolean did_dump;

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    cpl_error_code    ierror;
    cpl_errorstate    state_none;
    cpl_errorstate    state, state1;
    /* In order to produce a sequence of unique CPL errors,
       each call to cpl_error_set() is done with its own error code which
       is guatanteed not to originate from within CPL itself. */
    unsigned          icode, ncode = (unsigned)CPL_ERROR_EOL; 
    unsigned i;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_OFF);

    state_none = cpl_errorstate_get();

    /* Insert tests here */

    /* This is a test for DFS05408 */
    /* Set some errors */
    for (i=0; i < CPL_ERROR_HISTORY_SIZE/2; i++) {
        cpl_error_set(cpl_func, i + ncode);
    }

    /* Be able to recover to have only these errors */
    state1 = cpl_errorstate_get();

    /* Overflow the error history */
    for (i=0; i < 2 * CPL_ERROR_HISTORY_SIZE; i++) {
        cpl_error_set(cpl_func, CPL_ERROR_ILLEGAL_INPUT);
    }

    /* Recover to the original errors - the details of which have been lost */
    cpl_errorstate_set(state1);

    /* The dump should provide details only for the last error */
    cpl_errorstate_dump(state_none, CPL_FALSE, NULL);

    cpl_test_eq(cpl_error_get_code(), CPL_ERROR_HISTORY_LOST);

    /* Append one new error */
    cpl_error_set(cpl_func, CPL_ERROR_ILLEGAL_OUTPUT);

    /* The dump should provide details only for the last error */
    cpl_errorstate_dump(state_none, CPL_FALSE, NULL);

    cpl_test_eq(cpl_error_get_code(), CPL_ERROR_ILLEGAL_OUTPUT);
    cpl_errorstate_set(state1);

    /* The dump should provide no details */
    cpl_errorstate_dump(state_none, CPL_FALSE, NULL);

    cpl_test_eq(cpl_error_get_code(), CPL_ERROR_HISTORY_LOST);

    cpl_errorstate_set(state_none);

    /* Test 1: Verify that the error state is empty on start-up */
    cpl_test( cpl_errorstate_is_equal(state_none) );

    /* Test 1a: Verify that the current error cannot be used to
        modify the error state*/
    cpl_errorstate_set(state_none);
    cpl_test( cpl_errorstate_is_equal(state_none) );

    cpl_errorstate_dump(state_none, CPL_FALSE, NULL);
    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(state_none, CPL_FALSE, my_error_counter);
    cpl_test_eq( nerrors, 1 );
    cpl_test_eq( did_dump, CPL_FALSE );

    /* Test 2a: Detect a change in error state */
    cpl_error_set_message(cpl_func, ++ncode, "Hello, world");
    cpl_test_zero( cpl_errorstate_is_equal(state_none) );

    cpl_errorstate_dump(state_none, CPL_FALSE, NULL);
    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(state_none, CPL_FALSE, my_error_counter);
    cpl_test_eq( nerrors, 1 );
    cpl_test_eq( did_dump, CPL_TRUE );

    /* Test 2b: Try to dump zero errors with an existing errorstate */
    cpl_errorstate_dump(cpl_errorstate_get(), CPL_FALSE, NULL);

    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(cpl_errorstate_get(), CPL_FALSE, my_error_counter);
    cpl_test_eq( nerrors, 1 );
    cpl_test_eq( did_dump, CPL_FALSE );

    /* Add some errors without filling up the error history */
    for (ierror=1; ierror < CPL_ERROR_HISTORY_SIZE/2; ierror++) {
        ++ncode;
        cpl_error_set_message(cpl_func, ncode, "Error-code=%u",
                              (unsigned)ncode);
        cpl_test_eq( ncode, cpl_error_get_code() );
    }

    /* Preserve that error for later tests */
    state = cpl_errorstate_get();
    icode = ncode;

    cpl_errorstate_dump(state_none, CPL_FALSE, NULL);
    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(state_none, CPL_FALSE, my_error_counter);
    cpl_test_eq( nerrors, CPL_ERROR_HISTORY_SIZE/2 );
    cpl_test_eq( did_dump, CPL_TRUE );

    /* Test 3: Fill up the error history and check the 1st error */
    for (ierror=1; ierror < CPL_ERROR_HISTORY_SIZE; ierror++) {
        ++ncode;
        cpl_error_set_message(cpl_func, ncode, "Error-code=%u",
                              (unsigned)ncode);
        cpl_test_eq( ncode, cpl_error_get_code() );
    }

    cpl_test_eq( ncode + 1, icode + CPL_ERROR_HISTORY_SIZE );

    /* - error state is still non-empty */
    cpl_test_zero( cpl_errorstate_is_equal(state_none) );

    cpl_errorstate_dump(state_none, CPL_FALSE, NULL);
    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(state_none, CPL_FALSE, my_error_counter);
    cpl_test_eq( nerrors, CPL_ERROR_HISTORY_SIZE/2
                       + CPL_ERROR_HISTORY_SIZE - 1);
    cpl_test_eq( did_dump, CPL_TRUE );

    /* - different from the 1st error */
    cpl_test_zero( cpl_errorstate_is_equal(state) );

    cpl_errorstate_dump(state, CPL_FALSE, NULL);

    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(state, CPL_FALSE, my_error_counter);
    cpl_test_eq( nerrors, CPL_ERROR_HISTORY_SIZE - 1);
    cpl_test_eq( did_dump, CPL_TRUE );

    cpl_errorstate_set(state);

    cpl_errorstate_dump(state, CPL_FALSE, NULL);

    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(state, CPL_FALSE, my_error_counter);
    cpl_test_eq( nerrors, 1);
    cpl_test_eq( did_dump, CPL_FALSE );

    /* - the details of that 1st error are preserved */
    cpl_test_eq( cpl_error_get_code(), icode );
    cpl_test( cpl_error_get_line() != 0 );
    cpl_test_zero( strcmp(cpl_error_get_file(), __FILE__) );
    cpl_test_zero( strcmp(cpl_error_get_function(), cpl_func) );

    /* Set two new, 2nd and 3rd errors, discarding all errors set
       after 'state'. */

    ++ncode;
    cpl_error_set_message(cpl_func, ncode, "Error-code=%u", (unsigned)ncode);
    state1 = cpl_errorstate_get();
    ++ncode;
    cpl_error_set_message(cpl_func, ncode, "Error-code=%u", (unsigned)ncode);

    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(state, CPL_FALSE, my_error_counter);
    cpl_test_eq( nerrors, 2);
    cpl_test_eq( did_dump, CPL_TRUE );

    /* Preserve that error for later tests */
    state = cpl_errorstate_get();
    icode = ncode;

    cpl_errorstate_dump(state_none, CPL_FALSE, NULL);

    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(state_none, CPL_FALSE, my_error_counter);
    cpl_test_eq( nerrors, CPL_ERROR_HISTORY_SIZE/2 + 2);
    cpl_test_eq( did_dump, CPL_TRUE );

    /* Test 4: Fill up the error history and check that error */
    for (ierror=1; ierror < CPL_ERROR_HISTORY_SIZE; ierror++) {
        ++ncode;
        cpl_error_set_message(cpl_func, ncode, "Error-code=%u",
                              (unsigned)ncode);
        cpl_test_eq( ncode, cpl_error_get_code() );
    }
    cpl_test_eq( ncode + 1, icode + CPL_ERROR_HISTORY_SIZE );

    /* - error state is still non-empty */
    cpl_test_zero( cpl_errorstate_is_equal(state_none) );

    /* - different from the 3rd error */
    cpl_test_zero( cpl_errorstate_is_equal(state) );

    cpl_errorstate_dump(state_none, CPL_FALSE, NULL);

    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(state_none, CPL_FALSE, my_error_counter);
    cpl_test_eq( nerrors, CPL_ERROR_HISTORY_SIZE/2
                       + CPL_ERROR_HISTORY_SIZE + 1);
    cpl_test_eq( did_dump, CPL_TRUE );

    cpl_errorstate_set(state);

    /* - the details of that 3rd error are preserved */
    cpl_test_eq( cpl_error_get_code(), icode );
    cpl_test( cpl_error_get_line() != 0 );
    cpl_test_zero( strcmp(cpl_error_get_file(), __FILE__) );
    cpl_test_zero( strcmp(cpl_error_get_function(), cpl_func) );

    /* Test 5: Fill the error history and check the 3rd error */
    for (ierror=1; ierror < CPL_ERROR_HISTORY_SIZE; ierror++) {
        ++ncode;
        cpl_error_set_message(cpl_func, ncode, "Error-code=%u",
                              (unsigned)ncode);
        cpl_test_eq( ncode, cpl_error_get_code() );
    }
    cpl_test_eq( ncode + 2, icode + 2 * CPL_ERROR_HISTORY_SIZE );

    /* - and error state is still non-empty */
    cpl_test_zero( cpl_errorstate_is_equal(state_none) );

    /* - and different from the 3rd error */
    cpl_test_zero( cpl_errorstate_is_equal(state) );

    cpl_errorstate_dump(state_none, CPL_TRUE, NULL);

    nerrors = 0; did_dump = CPL_FALSE;
    cpl_errorstate_dump(state_none, CPL_TRUE, my_error_counter);
    cpl_test_eq( nerrors, CPL_ERROR_HISTORY_SIZE/2
                       + CPL_ERROR_HISTORY_SIZE + 1);
    cpl_test_eq( did_dump, CPL_TRUE );

    /* Test 6: Set the error state back to that 3rd error */
    cpl_errorstate_set(state);

    /* - error state is still non-empty */
    cpl_test_zero( cpl_errorstate_is_equal(state_none) );

    /* - and distinguiable from the 2nd error */
    cpl_test_zero( cpl_errorstate_is_equal(state1) );

    /* - and the details of that 3rd error are preserved */
    cpl_test_eq( cpl_error_get_code(), icode );
    cpl_test( cpl_error_get_line() != 0);
    cpl_test_zero( strcmp(cpl_error_get_file(), __FILE__) );
    cpl_test_zero( strcmp(cpl_error_get_function(), cpl_func) );

    /* Test 7: Verify that the details of the previous, 2nd error have
       been lost */

    cpl_errorstate_set(state1);

    /* - error state is still non-empty */
    cpl_test_zero( cpl_errorstate_is_equal(state_none) );

    /* - and distinguiable from the 3rd error */
    cpl_test_zero( cpl_errorstate_is_equal(state) );

    /* - but the details of that 2nd error has been lost */
    cpl_test_eq( cpl_error_get_code(), CPL_ERROR_HISTORY_LOST );
    cpl_test_zero( cpl_error_get_line());
    cpl_test_zero( strlen(cpl_error_get_file()) );
    cpl_test_zero( strlen(cpl_error_get_function()) );

    /* Test 8: Verify that error state cannot be advanced */
    cpl_errorstate_set(state);
    cpl_test( cpl_errorstate_is_equal(state1) );

    /* Test 8: Verify that error state can be cleared regardless */
    cpl_errorstate_set(state_none);
    cpl_test( cpl_errorstate_is_equal(state_none) );

    /* All tests are finished */
    return cpl_test_end(0);
}


/*----------------------------------------------------------------------------*/
/**
  @internal 
  @brief    Count the number of errors dumped
  @param    self      The number of the current error to be dumped
  @param    first     The number of the first error to be dumped
  @param    last      The number of the last error to be dumped
  @note Verifies also that the dumped errors are consequtive.
        Tries also to modify the CPL error state; if that were possible
        the counting should be corrupted.
  
 */
/*----------------------------------------------------------------------------*/
static void my_error_counter(unsigned self, unsigned first, unsigned last) {

    static cpl_errorstate prevstate; /* Don't try this at home */
    static unsigned preverror;

    if (self != 0) {
        if (self != first) {
            /* Error dump must be in steps of 1 */
            if (first < last) {
                cpl_test_eq( self, preverror + 1 );
            } else {
                cpl_test_eq( self, preverror - 1 );
            }
            /* Verify that the CPL error state cannot be modified from here */
            cpl_errorstate_set(prevstate);
        }

        preverror = self;
        prevstate = cpl_errorstate_get();
    }

    /* Verify that the CPL error state cannot be modified from here */
    cpl_error_reset();
    (void)cpl_error_set(cpl_func, CPL_ERROR_ILLEGAL_OUTPUT);
    cpl_error_set_where(cpl_func);

    nerrors++;

    if (first != 0 || last != 0) did_dump = CPL_TRUE;

}
