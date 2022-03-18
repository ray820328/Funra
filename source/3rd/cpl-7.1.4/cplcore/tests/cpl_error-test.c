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

#include <string.h>
#include <regex.h>

/*-----------------------------------------------------------------------------
                                  Prototypes of private functions 
 -----------------------------------------------------------------------------*/

static cpl_error_code cpl_error_test_set(cpl_error_code);
static cpl_error_code cpl_error_test_set_where(void);
static cpl_error_code cpl_error_test_set_message(cpl_error_code);
static cpl_error_code cpl_error_test_set_message_empty(cpl_error_code);
static cpl_error_code cpl_error_test_set_fits(cpl_error_code);
static cpl_error_code cpl_error_test_set_regex(cpl_error_code);
static cpl_error_code cpl_error_test_ensure(cpl_error_code);

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    const cpl_boolean has_func
        = strcmp(cpl_func, "main") ? CPL_FALSE : CPL_TRUE;
    cpl_error_code      ierror;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests here */

    /* Test 1: Verify that the error state is empty on start-up */

    cpl_test_zero( CPL_ERROR_NONE );
    cpl_test_eq( cpl_error_get_code(), CPL_ERROR_NONE );
    cpl_test_zero( cpl_error_get_line() );
    cpl_test_zero( strlen(cpl_error_get_function()) );
    cpl_test_zero( strlen(cpl_error_get_file()) );

    /* Test 1b: Verify that cpl_error_set() will not change that */

    cpl_test_eq( cpl_error_test_set(CPL_ERROR_NONE), CPL_ERROR_NONE );
    cpl_test_eq( cpl_error_get_code(), CPL_ERROR_NONE );
    cpl_test_zero( cpl_error_get_line() );
    cpl_test_zero( strlen(cpl_error_get_function()) );
    cpl_test_zero( strlen(cpl_error_get_file()) );

    /* Test 1c: Verify that cpl_error_set_where() will not change that */

    cpl_test_eq( cpl_error_test_set_where(), CPL_ERROR_NONE );
    cpl_test_eq( cpl_error_get_code(), CPL_ERROR_NONE );
    cpl_test_zero( cpl_error_get_line() );
    cpl_test_zero( strlen(cpl_error_get_function()) );
    cpl_test_zero( strlen(cpl_error_get_file()) );

    /* Test 1d: Verify that cpl_error_set_message() will not change that */

    cpl_test_eq( cpl_error_test_set_message(CPL_ERROR_NONE), CPL_ERROR_NONE);
    cpl_test_eq( cpl_error_get_code(), CPL_ERROR_NONE );
    cpl_test_zero( cpl_error_get_line() );
    cpl_test_zero( strlen(cpl_error_get_function()) );
    cpl_test_zero( strlen(cpl_error_get_file()) );

    /* Test 1e: Verify that cpl_error_set_fits() will not change that */
    cpl_test_eq( cpl_error_test_set_fits(CPL_ERROR_NONE), CPL_ERROR_NONE);
    cpl_test_eq( cpl_error_get_code(), CPL_ERROR_NONE );
    cpl_test_zero( cpl_error_get_line() );
    cpl_test_zero( strlen(cpl_error_get_function()) );
    cpl_test_zero( strlen(cpl_error_get_file()) );

    /* Test 1f: Verify that cpl_error_set_regex() will not change that */
    cpl_test_eq( cpl_error_test_set_regex(CPL_ERROR_NONE), CPL_ERROR_NONE);
    cpl_test_eq( cpl_error_get_code(), CPL_ERROR_NONE );
    cpl_test_zero( cpl_error_get_line() );
    cpl_test_zero( strlen(cpl_error_get_function()) );
    cpl_test_zero( strlen(cpl_error_get_file()) );

    /* Do a number of tests on all (other) error codes */

    for (ierror = CPL_ERROR_NONE; ierror <= CPL_ERROR_EOL+1; ierror++) {

        char msg[CPL_ERROR_MAX_MESSAGE_LENGTH];
        unsigned line;
        /* The expected error code */
        const cpl_error_code eerror = ierror == CPL_ERROR_HISTORY_LOST
            ? CPL_ERROR_UNSPECIFIED : ierror;

        /* Test 2:
           Verify that cpl_error_set_message() correctly sets the error */

        cpl_test_eq( cpl_error_test_set_message(ierror), eerror );
        cpl_test_eq( cpl_error_get_code(), eerror );

        /* - Except if the CPL error code is CPL_ERROR_NONE */
        if (ierror == CPL_ERROR_NONE) continue;

        cpl_test( cpl_error_get_line() > __LINE__ );
        cpl_test_eq_string(cpl_error_get_file(), __FILE__);
        cpl_test_eq_string(cpl_error_get_function(), "hardcoded");

        cpl_test_noneq_string(cpl_error_get_message(),
                              cpl_error_get_message_default(eerror));

        cpl_test_eq_ptr(strstr(cpl_error_get_message(),
                               cpl_error_get_message_default(eerror)),
                        cpl_error_get_message());

        strncpy(msg, cpl_error_get_message(), CPL_ERROR_MAX_MESSAGE_LENGTH);
        msg[CPL_ERROR_MAX_MESSAGE_LENGTH-1] = '\0';

        cpl_test( strlen(msg) > 0 );

        /* Test 3:
           Verify that cpl_error_set_message() correctly sets the error */

        cpl_test_eq( cpl_error_test_set_message_empty(ierror), eerror );
        cpl_test_eq( cpl_error_get_code(), eerror );
        cpl_test( cpl_error_get_line() > __LINE__ );
        cpl_test_eq_string(cpl_error_get_file(), __FILE__);
        cpl_test_eq_string(cpl_error_get_function(), "hardcoded");

        cpl_test_eq_string( cpl_error_get_message(),
                            cpl_error_get_message_default(eerror) );

        /* Test 4:
           Verify that cpl_error_set_fits() correctly sets the error */

        cpl_test_eq( cpl_error_test_set_fits(ierror), eerror );
        cpl_test_eq( cpl_error_get_code(), eerror );
        cpl_test( cpl_error_get_line() > __LINE__ );
        cpl_test_eq_string(cpl_error_get_file(), __FILE__);
        cpl_test_eq_string(cpl_error_get_function(), "cpl_error_test_set_fits");

        cpl_test_noneq_string(cpl_error_get_message(),
                              cpl_error_get_message_default(eerror));

        cpl_test_eq_ptr(strstr(cpl_error_get_message(),
                               cpl_error_get_message_default(eerror)),
                        cpl_error_get_message());

        /* Test 5: Verify that cpl_error_reset() correctly resets the error */
        cpl_error_reset();

        cpl_test_eq( cpl_error_get_code(), CPL_ERROR_NONE );
        cpl_test_zero( cpl_error_get_line() );
        cpl_test_zero( strlen(cpl_error_get_function()) );
        cpl_test_zero( strlen(cpl_error_get_file()) );

        /* Test 4a:
           Verify that cpl_error_set_regex() correctly sets the error */

        cpl_test_eq( cpl_error_test_set_regex(ierror), eerror );
        cpl_test_eq( cpl_error_get_code(), eerror );
        cpl_test( cpl_error_get_line() > __LINE__ );
        cpl_test_eq_string(cpl_error_get_file(), __FILE__);
        cpl_test_eq_string(cpl_error_get_function(), "cpl_error_test_set_regex");

        cpl_test_noneq_string(cpl_error_get_message(),
                              cpl_error_get_message_default(eerror));

        cpl_test_eq_ptr(strstr(cpl_error_get_message(),
                               cpl_error_get_message_default(eerror)),
                        cpl_error_get_message());

        /* Test 5a: Verify that cpl_error_reset() correctly resets the error */
        cpl_error_reset();

        cpl_test_eq( cpl_error_get_code(), CPL_ERROR_NONE );
        cpl_test_zero( cpl_error_get_line() );
        cpl_test_zero( strlen(cpl_error_get_function()) );
        cpl_test_zero( strlen(cpl_error_get_file()) );


        /* Test 6: Verify that cpl_ensure() correctly sets the error */

        cpl_test_eq( cpl_error_test_ensure(ierror), eerror );
        cpl_test_eq( cpl_error_get_code(), eerror );
        cpl_test( cpl_error_get_line() > __LINE__ );
        cpl_test_eq_string(cpl_error_get_file(), __FILE__);

        cpl_test_eq_ptr( strstr(msg, cpl_error_get_message()), msg );
        cpl_test( strlen(msg) > strlen(cpl_error_get_message()) );

        if (has_func)
            cpl_test_eq_string( cpl_error_get_function(),
                                "cpl_error_test_ensure" );

        line = cpl_error_get_line();

        /* Test 7: Verify that cpl_error_set_where() propagates correctly */

        cpl_test_eq( cpl_error_test_set_where(), eerror );
        cpl_test_eq( cpl_error_get_code(), eerror );
        cpl_test( cpl_error_get_line() > __LINE__ );
        cpl_test_noneq( cpl_error_get_line(), line );
        cpl_test_eq_string(cpl_error_get_file(), __FILE__);

        cpl_test( strstr(msg, cpl_error_get_message()) == msg );
        cpl_test( strlen(msg) > strlen(cpl_error_get_message()) );

        if (has_func)
            cpl_test_eq_string( cpl_error_get_function(), "where" );


        /* Test 8: Verify that cpl_error_set() correctly sets the error */

        cpl_test_eq( cpl_error_test_set(ierror), eerror );
        cpl_test_eq( cpl_error_get_code(), eerror );
        cpl_test( cpl_error_get_line() > __LINE__ );
        cpl_test_eq_string(cpl_error_get_file(), __FILE__);

        cpl_test( strstr(msg, cpl_error_get_message()) == msg );
        cpl_test( strlen(msg) > strlen(cpl_error_get_message()) );

        if (has_func)
            cpl_test_eq_string( cpl_error_get_function(), "set" );

        /* Test 9: Verify that cpl_error_reset() correctly resets the error */
        cpl_error_reset();

        cpl_test_eq( cpl_error_get_code(), CPL_ERROR_NONE );
        cpl_test_zero( cpl_error_get_line() );
        cpl_test_zero( strlen(cpl_error_get_function()) );
        cpl_test_zero( strlen(cpl_error_get_file()) );

    }

    /* End of actual test code */
    return cpl_test_end(0);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Set the error using the supplied code and cpl_error_set()
  @param  code   The error code to set
  @return The error code

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_error_test_set(cpl_error_code code)
{
    return cpl_error_set("set", code);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Propagate the CPL error
  @return The error code

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_error_test_set_where(void)
{

    const cpl_error_code error = cpl_error_set_where("where");
    const char * where = cpl_error_get_where();

    if (error) {
        cpl_test_eq_ptr(where, strstr(where, "where"));
        cpl_test_nonnull(strstr(where, __FILE__));
    } else {
        cpl_test_null(strstr(where, "where"));
        cpl_test_null(strstr(where, __FILE__));
    }

    return error;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Set the error using the supplied code and cpl_error_set()
  @param  code   The error code to set
  @return The error code

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_error_test_set_message(cpl_error_code code)
{

    return cpl_error_set_message("hardcoded", code, "Error-code=%u",
                                 (unsigned)code);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Set the error using the supplied code and cpl_error_set()
  @param  code   The error code to set
  @return The error code

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_error_test_set_fits(cpl_error_code code)
{

    return cpl_error_set_fits(code, 0, "cfitsio_dummy", "Error-code=%u",
                              (unsigned)code);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Set the error using the supplied code and cpl_error_set()
  @param  code   The error code to set
  @return The error code

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_error_test_set_regex(cpl_error_code code)
{

    regex_t re;
    const int errcode = regcomp(&re, ")|(", REG_EXTENDED | REG_NOSUB);
    const cpl_error_code rcode
        = cpl_error_set_regex(code, errcode,
                              code == CPL_ERROR_NULL_INPUT ? NULL : &re,
                              "Error-code=%u", (unsigned)code);
    regfree(&re);

    return rcode;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Set the error using the supplied code and cpl_error_set()
  @param  code   The error code to set
  @return The error code

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_error_test_set_message_empty(cpl_error_code code)
{
    return cpl_error_set_message("hardcoded", code, " ");
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Set the error using the supplied code and cpl_error_ensure()
  @param  code   The error code to set
  @return The error code

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_error_test_ensure(cpl_error_code code)
{

    cpl_ensure_code(0, code);

}
