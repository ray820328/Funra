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


AM_CPPFLAGS = $(CPLDFS_INCLUDES) $(CPLCORE_INCLUDES) \
              $(CPLUI_INCLUDES) $(CX_INCLUDES) $(CFITSIO_INCLUDES)

LDADD = $(LIBCPLDFS) $(LIBCPLUI) $(LIBCPLCORE) $(LIBCEXT)


check_PROGRAMS = cpl_dfs-test

cpl_dfs_test_SOURCES = cpl_dfs-test.c

# Be sure to reexport important environment variables.
TESTS_ENVIRONMENT = MAKE="$(MAKE)" CC="$(CC)" CFLAGS="$(CFLAGS)" \
	CPPFLAGS="$(CPPFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS)" \
	LIBS="$(LIBS)" LN_S="$(LN_S)" NM="$(NM)" RANLIB="$(RANLIB)" \
	OBJEXT="$(OBJEXT)" EXEEXT="$(EXEEXT)"


TESTS = cpl_dfs-test

XFAIL_TESTS =

# We need to remove any files that the above tests created.
clean-local:
	$(RM) *.fits *.paf *.log
