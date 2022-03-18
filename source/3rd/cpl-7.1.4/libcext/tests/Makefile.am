## Process this file with automake to produce Makefile.in

## This file is part of the ESO C Extension Library
## Copyright (C) 2001-2017 European Southern Observatory

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


AUTOMAKE_OPTIONS = 1.8 foreign

DISTCLEANFILES = *~


if MAINTAINER_MODE

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

endif


EXTRA_DIST =

LIBCEXT = $(top_builddir)/cext/libcext.la

AM_CPPFLAGS = -I$(top_srcdir)/cext -I$(top_builddir)/cext
AM_LDFLAGS = -static

LDADD = $(LIBCEXT)

check_PROGRAMS = cxslist-test cxlist-test cxtree-test cxmap-test \
	cxstring-test cxdeque-test cxtypes-test

cxslist_test_SOURCES = cxslist-test.c
cxlist_test_SOURCES = cxlist-test.c
cxtree_test_SOURCES = cxtree-test.c
cxmap_test_SOURCES = cxmap-test.c
cxstring_test_SOURCES = cxstring-test.c
cxdeque_test_SOURCES = cxdeque-test.c
cxtypes_test_SOURCES = cxtypes-test.c


# Be sure to reexport important environment variables.
TESTS_ENVIRONMENT = MAKE="$(MAKE)" CC="$(CC)" CFLAGS="$(CFLAGS)" \
	CPPFLAGS="$(CPPFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS)" \
	LIBS="$(LIBS)" LN_S="$(LN_S)" NM="$(NM)" RANLIB="$(RANLIB)" \
	OBJEXT="$(OBJEXT)" EXEEXT="$(EXEEXT)"


TESTS = cxslist-test cxlist-test cxtree-test cxmap-test cxstring-test \
	cxdeque-test cxtypes-test

XFAIL_TESTS =

# We need to remove any files that the above tests created.
clean-local:
