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

#include "cpl_test.h"
#include "cpl_memory.h"
#include "cpl_plugin.h"

#include <stdio.h>

int main(void)
{

    cpl_plugin    *plgn1;

    int            ecode;

    const unsigned int api1 = 1;
    unsigned int   aapi1;


    const unsigned long version1 = 30507L,
                   type1 = 1L;

    unsigned long  aversion1,
                   atype1;

    char          *asversion1, *astype1;
    const char    *asnull;
    const char    *sversion1 = "3.5.7",
                  *stype1 = "recipe",
                  *sname1 = "eso.cpl.test",
                  *asname1,
                  *ssynopsis1 = "test_synopsis",
                  *assynopsis1,
                  *sdescription1 = "test_description",
                  *asdescription1,
                  *sauthor1 = "test_author",
                  *asauthor1,
                  *semail1 = "test_email",
                  *asemail1,
                  *scopyright1 = "test_copyright",
                  *ascopyright1;

    cpl_plugin_func  initialize1 = (cpl_plugin_func)1,
                     ainitialize1,
                     execute1 = (cpl_plugin_func)1,
                     aexecute1,
                     deinitialize1 = (cpl_plugin_func)1,
                     adeinitialize1;

    FILE        * stream;



    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);


    /*
     *  Test1 : Accessor Functions
     */

    stream = cpl_msg_get_level() > CPL_MSG_INFO
        ? fopen("/dev/null", "a") : stdout;

    cpl_test_nonnull(stream);

    plgn1 = cpl_plugin_new();

    cpl_test_nonnull(plgn1);

    /* PLUGIN API */

    ecode = cpl_plugin_set_api(NULL, api1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    aapi1 =  cpl_plugin_get_api(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_zero(aapi1);

    ecode = cpl_plugin_set_api(plgn1, api1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    aapi1 = cpl_plugin_get_api(plgn1);
    cpl_test_eq(aapi1, api1);

    /* PLUGIN VERSION */

    ecode = cpl_plugin_set_version(NULL, version1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    aversion1 =  cpl_plugin_get_version(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_zero(aversion1);

    asnull = cpl_plugin_get_version_string(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(asnull);

    ecode = cpl_plugin_set_version(plgn1, version1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    aversion1 =  cpl_plugin_get_version(plgn1);

    cpl_test_eq(aversion1, version1);

    asversion1 = cpl_plugin_get_version_string(plgn1);

    cpl_test_noneq_ptr(sversion1, asversion1);
    cpl_test_eq_string(sversion1, asversion1);

    cpl_free(asversion1);

    /* PLUGIN TYPE */

    ecode = cpl_plugin_set_type(NULL, type1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    atype1 =  cpl_plugin_get_type(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_eq(atype1, CPL_PLUGIN_TYPE_NONE);

    asnull = cpl_plugin_get_type_string(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(asnull);

    ecode = cpl_plugin_set_type(plgn1, type1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    atype1 =  cpl_plugin_get_type(plgn1);

    cpl_test_eq(atype1, type1);

    astype1 = cpl_plugin_get_type_string(plgn1);

    cpl_test_noneq_ptr(stype1, astype1);
    cpl_test_eq_string(stype1, astype1);

    cpl_free(astype1);

    /* PLUGIN NAME */

    ecode = cpl_plugin_set_name(NULL, sname1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_set_name(plgn1, NULL);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    asnull =  cpl_plugin_get_name(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(asnull);

    ecode = cpl_plugin_set_name(plgn1, sname1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    asname1 =  cpl_plugin_get_name(plgn1);
    cpl_test_noneq_ptr(sname1, asname1);
    cpl_test_eq_string(sname1, asname1);

    /* PLUGIN SYNOPSIS */

    ecode = cpl_plugin_set_synopsis(NULL, ssynopsis1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    asnull =  cpl_plugin_get_synopsis(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(asnull);

    ecode = cpl_plugin_set_synopsis(plgn1, NULL);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    asnull =  cpl_plugin_get_synopsis(plgn1);
    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_null(asnull);

    ecode = cpl_plugin_set_synopsis(plgn1, ssynopsis1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    assynopsis1 =  cpl_plugin_get_synopsis(plgn1);
    cpl_test_noneq_ptr(ssynopsis1, assynopsis1);
    cpl_test_eq_string(ssynopsis1, assynopsis1);

    /* PLUGIN DESCRIPTION */

    ecode = cpl_plugin_set_description(NULL, sdescription1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    asnull =  cpl_plugin_get_description(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(asnull);

    ecode = cpl_plugin_set_description(plgn1, NULL);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    asnull =  cpl_plugin_get_description(plgn1);
    cpl_test_null(asnull);

    ecode = cpl_plugin_set_description(plgn1, sdescription1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    asdescription1 =  cpl_plugin_get_description(plgn1);
    cpl_test_noneq_ptr(sdescription1, asdescription1);
    cpl_test_eq_string(sdescription1, asdescription1);

    /* PLUGIN AUTHOR */

    ecode = cpl_plugin_set_author(NULL, sauthor1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_set_author(plgn1, NULL);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    asnull =  cpl_plugin_get_author(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(asnull);

    ecode = cpl_plugin_set_author(plgn1, sauthor1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    asauthor1 =  cpl_plugin_get_author(plgn1);
    cpl_test_noneq_ptr(sauthor1, asauthor1);
    cpl_test_eq_string(sauthor1, asauthor1);

    /* PLUGIN EMAIL */

    ecode = cpl_plugin_set_email(NULL, semail1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_set_email(plgn1, NULL);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    asnull =  cpl_plugin_get_email(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(asnull);

    ecode = cpl_plugin_set_email(plgn1, semail1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    asemail1 =  cpl_plugin_get_email(plgn1);
    cpl_test_noneq_ptr(semail1, asemail1);
    cpl_test_eq_string(semail1, asemail1);

    /* PLUGIN COPYRIGHT */

    ecode = cpl_plugin_set_copyright(NULL, scopyright1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_set_copyright(plgn1, NULL);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    asnull =  cpl_plugin_get_copyright(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(asnull);

    ecode = cpl_plugin_set_copyright(plgn1, scopyright1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    ascopyright1 =  cpl_plugin_get_copyright(plgn1);
    cpl_test_noneq_ptr(scopyright1, ascopyright1);
    cpl_test_eq_string(scopyright1, ascopyright1);

    /* PLUGIN INITIALIZE */

    ecode = cpl_plugin_set_init(NULL, initialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_set_init(plgn1, NULL);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    ainitialize1 =  cpl_plugin_get_init(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test(ainitialize1 == NULL);

    ainitialize1 =  cpl_plugin_get_init(plgn1);
    cpl_test(ainitialize1 == NULL);

    ecode = cpl_plugin_set_init(plgn1, initialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    ainitialize1 =  cpl_plugin_get_init(plgn1);
    cpl_test(ainitialize1 == initialize1);

    /* PLUGIN EXECUTE */

    ecode = cpl_plugin_set_exec(NULL, execute1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    aexecute1 =  cpl_plugin_get_exec(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test(aexecute1 == NULL);

    ecode = cpl_plugin_set_exec(plgn1, NULL);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    aexecute1 =  cpl_plugin_get_exec(plgn1);
    cpl_test(aexecute1 == NULL);

    ecode = cpl_plugin_set_exec(plgn1, execute1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    aexecute1 =  cpl_plugin_get_exec(plgn1);
    cpl_test(aexecute1 == execute1);

    /* PLUGIN DEINITIALIZE */

    ecode = cpl_plugin_set_deinit(NULL, deinitialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    adeinitialize1 =  cpl_plugin_get_deinit(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test(adeinitialize1 == NULL);

    ecode = cpl_plugin_set_deinit(plgn1, NULL);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    adeinitialize1 =  cpl_plugin_get_deinit(plgn1);
    cpl_test(adeinitialize1 == NULL);

    ecode = cpl_plugin_set_deinit(plgn1, deinitialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    adeinitialize1 =  cpl_plugin_get_deinit(plgn1);
    cpl_test(adeinitialize1 == deinitialize1);

    cpl_plugin_delete(plgn1);

    plgn1 = cpl_plugin_new();
    cpl_test_nonnull(plgn1);

    /* PLUGIN INIT */

    ecode = cpl_plugin_init(NULL,
                            api1,
                            version1,
                            type1,
                            sname1,
                            ssynopsis1,
                            sdescription1,
                            sauthor1,
                            semail1,
                            scopyright1,
                            initialize1,
                            execute1,   
                            deinitialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_init(plgn1,
                            api1,
                            version1,
                            type1,
                            NULL,
                            ssynopsis1,
                            sdescription1,
                            sauthor1,
                            semail1,
                            scopyright1,
                            initialize1,
                            execute1,   
                            deinitialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_init(plgn1,
                            api1,
                            version1,
                            type1,
                            sname1,
                            NULL,
                            sdescription1,
                            sauthor1,
                            semail1,
                            scopyright1,
                            initialize1,
                            execute1,   
                            deinitialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_init(plgn1,
                            api1,
                            version1,
                            type1,
                            sname1,
                            ssynopsis1,
                            NULL,
                            sauthor1,
                            semail1,
                            scopyright1,
                            initialize1,
                            execute1,   
                            deinitialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_init(plgn1,
                            api1,
                            version1,
                            type1,
                            sname1,
                            ssynopsis1,
                            sdescription1,
                            NULL,
                            semail1,
                            scopyright1,
                            initialize1,
                            execute1,   
                            deinitialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_init(plgn1,
                            api1,
                            version1,
                            type1,
                            sname1,
                            ssynopsis1,
                            sdescription1,
                            sauthor1,
                            NULL,
                            scopyright1,
                            initialize1,
                            execute1,   
                            deinitialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_init(plgn1,
                            api1,
                            version1,
                            type1,
                            sname1,
                            ssynopsis1,
                            sdescription1,
                            sauthor1,
                            semail1,
                            NULL,
                            initialize1,
                            execute1,   
                            deinitialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);

    ecode = cpl_plugin_init(plgn1,
                            api1,
                            version1,
                            type1,
                            sname1,
                            ssynopsis1,
                            sdescription1,
                            sauthor1,
                            semail1,
                            scopyright1,
                            initialize1,
                            execute1,   
                            deinitialize1);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    /* PLUGIN API */

    aapi1 =  cpl_plugin_get_api(plgn1);
    cpl_test_eq(aapi1, api1);

    /* PLUGIN VERSION */

    aversion1 =  cpl_plugin_get_version(plgn1);
    cpl_test_eq(aversion1, version1);

    asversion1 = cpl_plugin_get_version_string(plgn1);
    cpl_test_noneq_ptr(asversion1, sversion1);
    cpl_test_eq_string(asversion1, sversion1);

    cpl_free(asversion1);

    /* PLUGIN TYPE */

    atype1 =  cpl_plugin_get_type(plgn1);
    cpl_test_eq(atype1, type1);

    astype1 = cpl_plugin_get_type_string(plgn1);
    cpl_test_noneq_ptr(stype1,astype1);
    cpl_test_eq_string(stype1,astype1);

    cpl_free(astype1);

    /* PLUGIN NAME */

    asname1 =  cpl_plugin_get_name(plgn1);
    cpl_test_noneq_ptr(sname1, asname1);
    cpl_test_eq_string(sname1, asname1);


    /* PLUGIN SYNOPSIS */

    assynopsis1 =  cpl_plugin_get_synopsis(plgn1);
    cpl_test_noneq_ptr(ssynopsis1, assynopsis1);
    cpl_test_eq_string(ssynopsis1, assynopsis1);


    /* PLUGIN DESCRIPTION */

    asdescription1 =  cpl_plugin_get_description(plgn1);
    cpl_test_noneq_ptr(sdescription1, asdescription1);
    cpl_test_eq_string(sdescription1, asdescription1);

    /* PLUGIN AUTHOR */

    asauthor1 =  cpl_plugin_get_author(plgn1);
    cpl_test_noneq_ptr(sauthor1, asauthor1);
    cpl_test_eq_string(sauthor1, asauthor1);

    /* PLUGIN EMAIL */

    asemail1 =  cpl_plugin_get_email(plgn1);
    cpl_test_noneq_ptr(semail1,asemail1);
    cpl_test_eq_string(semail1,asemail1);

    /* PLUGIN COPYRIGHT */

    ascopyright1 =  cpl_plugin_get_copyright(plgn1);
    cpl_test_noneq_ptr(scopyright1, ascopyright1);
    cpl_test_eq_string(scopyright1, ascopyright1);


    /* PLUGIN INITIALIZE */

    ainitialize1 =  cpl_plugin_get_init(plgn1);
    cpl_test(ainitialize1 == initialize1);


    /* PLUGIN EXECUTE */

    aexecute1 =  cpl_plugin_get_exec(plgn1);
    cpl_test(aexecute1 == execute1);


    /* PLUGIN DEINITIALIZE */

    adeinitialize1 =  cpl_plugin_get_deinit(plgn1);
    cpl_test(adeinitialize1 == deinitialize1);

    cpl_plugin_dump(NULL, stream);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_plugin_dump(plgn1, stream);
    cpl_test_error(CPL_ERROR_NONE);

    cpl_plugin_delete(plgn1);

    if (stream != stdout) cpl_test_zero( fclose(stream) );

    /*
     * All tests succeeded
     */

    return cpl_test_end(0);

}
 
