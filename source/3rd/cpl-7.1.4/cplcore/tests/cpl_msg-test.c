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


/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_msg.h"
#include "cpl_test.h"
#include "cpl_memory.h"

#include <string.h>

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

#ifndef STR_LENGTH    
#define STR_LENGTH 80
#endif

/*-----------------------------------------------------------------------------
                                  Main
 -----------------------------------------------------------------------------*/
int main(void)
{
    char message[STR_LENGTH+1];
    char toolong[CPL_MAX_MSG_LENGTH];
    char* verylongname = NULL;
    int verylongnamesize = 1024*1024;
    int nindent = 0;
    cpl_msg_severity loglevel = CPL_MSG_OFF;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    /* Construct the null-terminated string of STR_LENGTH dots */
    (void)memset((void*)message, '.', STR_LENGTH);
    message[STR_LENGTH] = '\0';

    /* - and display it */
    cpl_msg_info(cpl_func, "Display a string of %d dots: %s", STR_LENGTH,
                 message);
    cpl_msg_info(cpl_func, "Domain is: %s", cpl_msg_get_domain());

    cpl_msg_indent_more(); nindent++;
    cpl_msg_info(cpl_func,"hhu: %hhu\n", (unsigned char)-1);
    cpl_msg_indent_more(); nindent++;
    cpl_msg_info(cpl_func,"hhd: %hhd\n", (char)-1);
    cpl_msg_indent_more(); nindent++;
    cpl_msg_info(cpl_func,"zu: %zu\n", sizeof(size_t));
    cpl_msg_indent_more(); nindent++;
    cpl_msg_info(cpl_func,"td: %td\n",  (__func__ ) - (__func__ + 2) );
    cpl_msg_indent_more(); nindent++;
    cpl_msg_info(cpl_func,"llu: %llu\n", (unsigned long long)-1);
    cpl_msg_indent_more(); nindent++;
    cpl_msg_info(cpl_func,"lld: %lld\n", (long long)-1);
    cpl_msg_indent_more(); nindent++;

    /* Test a too long message */

    /* Construct the null-terminated string of STR_LENGTH dots */
    (void)memset((void*)toolong, '.', CPL_MAX_MSG_LENGTH-1);
    toolong[CPL_MAX_MSG_LENGTH-1] = '\0';

    cpl_msg_info(cpl_func, "Display a string of %d dots: %s",
                 CPL_MAX_MSG_LENGTH-1, toolong);

    cpl_msg_set_time_on();

    for (;nindent;  nindent--)
        cpl_msg_indent_less(); /* Undo the indenting */

    /* Construct a very long null-terminated string of 'A' characters */
    verylongname = (char*) cpl_malloc(verylongnamesize);
    cpl_test_nonnull(verylongname);
    (void)memset((void*)verylongname, 'A', verylongnamesize-1);
    verylongname[verylongnamesize-1] = '\0';
    
    loglevel = cpl_msg_get_level();
    cpl_msg_set_level(CPL_MSG_OFF);
    cpl_msg_debug(verylongname, "test for long component name");
    cpl_msg_set_level(loglevel);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_msg_set_level(CPL_MSG_OFF);
    cpl_msg_info(verylongname, "test for long component name");
    cpl_msg_set_level(loglevel);
    cpl_msg_set_level(loglevel);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_msg_set_level(CPL_MSG_OFF);
    cpl_msg_warning(verylongname, "test for long component name");
    cpl_msg_set_level(loglevel);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_msg_set_level(CPL_MSG_OFF);
    cpl_msg_error(verylongname, "test for long component name");
    cpl_msg_set_level(loglevel);
    cpl_test_error(CPL_ERROR_NONE);

#ifdef CPL_MSG_TEST_DEPRECATED
    for (int i = 0; i < 1000; i++) {
CPL_DIAG_PRAGMA_PUSH_IGN(-Wdeprecated-declarations);
        cpl_msg_progress(cpl_func, i, 1000, "test: %d/%d", i, 1000);
CPL_DIAG_PRAGMA_POP;
    }
#endif
    
    cpl_free(verylongname);

    /* End of tests */
    return cpl_test_end(0);

}
