## Process this file with automake to produce Makefile.in

##   This file is part of the ESO Common Pipeline Library
##   Copyright (C) 2001-2017 European Southern Observatory

##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as published by
##   the Free Software Foundation; either version 2 of the License, or
##   (at your option) any later version.

##   This program is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.

##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307  USA


AUTOMAKE_OPTIONS = 1.8 foreign

DISTCLEANFILES = *~


if MAINTAINER_MODE

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

endif


LDADD = $(LIBCPLUI) $(LIBCPLCORE) $(LIBCFITSIO) $(LIBCEXT)

AM_CPPFLAGS = $(CPLUI_INCLUDES) $(CPLCORE_INCLUDES) $(CX_INCLUDES) \
              $(CFITSIO_INCLUDES)
AM_LDFLAGS = $(CFITSIO_LDFLAGS)


check_PROGRAMS = cpl_frame-test \
                 cpl_frameset-test \
                 cpl_frameset_io-test \
                 cpl_parameter-test \
                 cpl_parameterlist-test \
                 cpl_plugin-test \
                 cpl_pluginlist-test \
                 cpl_framedata-test \
                 cpl_recipedefine-test \
                 cpl_recipeconfig-test

cpl_frame_test_SOURCES = cpl_frame-test.c
cpl_frameset_test_SOURCES = cpl_frameset-test.c
cpl_frameset_io_test_SOURCES = cpl_frameset_io-test.c
cpl_parameter_test_SOURCES = cpl_parameter-test.c
cpl_parameterlist_test_SOURCES = cpl_parameterlist-test.c
cpl_plugin_test_SOURCES = cpl_plugin-test.c
cpl_pluginlist_test_SOURCES = cpl_pluginlist-test.c
cpl_framedata_test_SOURCES = cpl_framedata-test.c
cpl_recipedefine_test_SOURCES = cpl_recipedefine-test.c
cpl_recipeconfig_test_SOURCES = cpl_recipeconfig-test.c

# Be sure to reexport important environment variables.
TESTS_ENVIRONMENT = MAKE="$(MAKE)" CC="$(CC)" CFLAGS="$(CFLAGS)" \
	CPPFLAGS="$(CPPFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS)" \
	LIBS="$(LIBS)" LN_S="$(LN_S)" NM="$(NM)" RANLIB="$(RANLIB)" \
	OBJEXT="$(OBJEXT)" EXEEXT="$(EXEEXT)"


TESTS = cpl_frame-test \
        cpl_frameset-test \
        cpl_frameset_io-test \
        cpl_parameter-test \
        cpl_parameterlist-test \
        cpl_plugin-test \
        cpl_pluginlist-test \
        cpl_framedata-test \
        cpl_recipedefine-test \
        cpl_recipeconfig-test

XFAIL_TESTS =

# We need to remove any files that the above tests created.
clean-local:
	$(RM) *.fits *.tfits *.log *dump.txt
