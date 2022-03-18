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
#include "cpl_pluginlist.h"
#include "cpl_recipedefine.h"

int main(void)
{

    const char * eso_gpl_license = cpl_get_license(PACKAGE_NAME, "2005, 2008");

    cpl_plugin     *pl1    = NULL,
                   *pl2    = NULL,
                   *pl3    = NULL,
                   *pl4    = NULL,
                   *pl5    = NULL,
                   *front  = NULL,
                   *back   = NULL,
                   *search = NULL;

    cpl_pluginlist *pllst  = NULL;

    cpl_error_code error;


    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    cpl_test_nonnull(eso_gpl_license);

    pl1 = cpl_plugin_new();
    cpl_test_nonnull(pl1);

    error = cpl_plugin_init(pl1,
                            1,
                            10000,
                            (unsigned long) CPL_PLUGIN_TYPE_RECIPE,
                            "cmdlcaller.test.plgn1",
                            "synopsis1",
                            "description1",
                            "Fname Lname",
                            PACKAGE_BUGREPORT,
                            eso_gpl_license,
                            NULL,
                            NULL,
                            NULL);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    pl2 = cpl_plugin_new();
    cpl_test_nonnull(pl2);

    error = cpl_plugin_init(pl2,
                            1,
                            10000,
                            (unsigned long) CPL_PLUGIN_TYPE_RECIPE,
                            "cmdlcaller.test.plgn2",
                            "synopsis2",
                            "description2",
                            "Fname Lname",
                            PACKAGE_BUGREPORT,
                            eso_gpl_license,
                            NULL,
                            NULL,
                            NULL);
    cpl_test_eq_error(error, CPL_ERROR_NONE);


    pl3 = cpl_plugin_new();
    cpl_test_nonnull(pl3);

    error = cpl_plugin_init(pl3,
                            1,
                            10000,
                            (unsigned long) CPL_PLUGIN_TYPE_RECIPE,
                            "cmdlcaller.test.plgn3",
                            "synopsis3",
                            "description3",
                            "Fname Lname",
                            PACKAGE_BUGREPORT,
                            eso_gpl_license,
                            NULL,
                            NULL,
                            NULL);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    pl4 = cpl_plugin_new();
    cpl_test_nonnull(pl4);

    error = cpl_plugin_init(pl4,
                            1,
                            10000,
                            (unsigned long) CPL_PLUGIN_TYPE_RECIPE,
                            "cmdlcaller.test.plgn4",
                            "synopsis4",
                            "description4",
                            "Fname Lname",
                            PACKAGE_BUGREPORT,
                            eso_gpl_license,
                            NULL,
                            NULL,
                            NULL);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    pl5 = cpl_plugin_new();
    cpl_test_nonnull(pl5);

    error = cpl_plugin_init(pl5,
                            1,
                            10000,
                            (unsigned long) CPL_PLUGIN_TYPE_RECIPE,
                            "cmdlcaller.test.plgn5",
                            "synopsis5",
                            "description5",
                            "Fname Lname",
                            PACKAGE_BUGREPORT,
                            eso_gpl_license,
                            NULL,
                            NULL,
                            NULL);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    pllst = cpl_pluginlist_new();
    cpl_test_nonnull(pllst);


    /*
     * After appending and prepending the ordering of the plugins
     * is like this: 4 5 1 3 2
     */

    error = cpl_pluginlist_append(pllst, pl1);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_pluginlist_get_size(pllst), 1);

    error = cpl_pluginlist_append(pllst, pl3);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_pluginlist_get_size(pllst), 2);

    error = cpl_pluginlist_append(pllst, pl2);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_pluginlist_get_size(pllst), 3);

    error = cpl_pluginlist_prepend(pllst, pl5);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_pluginlist_get_size(pllst), 4);

    error = cpl_pluginlist_prepend(pllst, pl4);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_eq(cpl_pluginlist_get_size(pllst), 5);

    /*
     *
     */


    front  = cpl_pluginlist_get_first(pllst);
    back   = cpl_pluginlist_get_last(pllst);

    cpl_test_noneq_ptr(front, back);

    cpl_test_eq_string(cpl_plugin_get_name(front), "cmdlcaller.test.plgn4");
    cpl_test_eq_string(cpl_plugin_get_name(back), "cmdlcaller.test.plgn2");

    search = cpl_pluginlist_find(pllst, "cmdlcaller.test.plgn2");

    cpl_test_eq_ptr(search, back);

    cpl_test_eq_string(cpl_plugin_get_description(search), "description2");

    cpl_pluginlist_delete(pllst);

    return cpl_test_end(0);

}
 
