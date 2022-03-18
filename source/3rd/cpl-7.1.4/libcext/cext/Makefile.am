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

DISTCLEANFILES = *~ cxconfig-stamp cxconfig.h


if MAINTAINER_MODE

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in $(BUILT_SOURCES)

endif

AM_CPPFLAGS = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500 \
	$(CEXT_INCLUDES) @CX_DEBUG_FLAGS@

BUILT_SOURCES = cxconfig-stamp

include_HEADERS = cxdeque.h cxfileutils.h cxlist.h cxmap.h \
	cxmacros.h cxmemory.h cxmessages.h cxmultimap.h cxslist.h \
	cxstrutils.h cxtree.h cxutils.h cxstring.h cxtypes.h
	
noinst_HEADERS = cxthread.h
nodist_config_HEADERS = cxconfig.h

lib_LTLIBRARIES = libcext.la

libcext_la_SOURCES = cxfileutils.c cxlist.c cxmap.c cxmemory.c \
	cxmessages.c cxmultimap.c cxslist.c cxstring.c cxstrutils.c cxtree.c \
	cxutils.c cxdeque.c 

EXTRA_libcext_la_SOURCES = snprintf.h snprintf.c

libcext_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libcext_la_LIBADD = @SNPRINTF@
libcext_la_DEPENDENCIES = @SNPRINTF@


cxconfig-stamp: $(top_builddir)/config.status
	$(AM_V_GEN) cd $(top_builddir) && $(SHELL) ./config.status cxconfig.h
	@touch cxconfig-stamp
