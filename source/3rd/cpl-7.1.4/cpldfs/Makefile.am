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

SUBDIRS = tests

AM_CPPFLAGS = -DCX_LOG_DOMAIN=\"CplDfs\" \
	          $(CPLDFS_INCLUDES) $(CPLUI_INCLUDES) \
              $(CPLCORE_INCLUDES) $(CX_INCLUDES) $(CFITSIO_INCLUDES)

include_HEADERS = cpl_dfs.h cpl_multiframe.h

noinst_HEADERS = md5.h md5.c

lib_LTLIBRARIES = libcpldfs.la

libcpldfs_la_SOURCES = cpl_dfs.c cpl_multiframe.c

libcpldfs_la_LDFLAGS = $(CX_LDFLAGS) $(CFITSIO_LDFLAGS) -version-info \
                       $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libcpldfs_la_LIBADD =  $(LIBCPLUI) $(LIBCPLCORE) $(LIBCFITSIO) \
                       $(LIBCEXT) -lm
libcpldfs_la_DEPENDENCIES = $(LIBCPLCORE)

cpl_dfs.c: $(srcdir)/md5.h $(srcdir)/md5.c
