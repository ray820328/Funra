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

AM_CPPFLAGS = -DCX_LOG_DOMAIN=\"CplUi\" \
              $(CPLCORE_INCLUDES) $(CX_INCLUDES) $(CFITSIO_INCLUDES)

include_HEADERS = cpl_frame.h cpl_frameset.h cpl_frameset_io.h \
                  cpl_parameter.h cpl_parameterlist.h cpl_plugin.h \
                  cpl_plugininfo.h cpl_pluginlist.h cpl_recipe.h \
                  cpl_recipedefine.h cpl_recipeconfig.h cpl_framedata.h

noinst_HEADERS = cpl_frame_impl.h 

lib_LTLIBRARIES = libcplui.la

libcplui_la_SOURCES = cpl_frame.c cpl_frameset.c cpl_frameset_io.c \
                      cpl_parameter.c cpl_parameterlist.c cpl_plugin.c \
                      cpl_recipedefine.c  \
                      cpl_pluginlist.c cpl_recipeconfig.c cpl_framedata.c

libcplui_la_LDFLAGS = $(CX_LDFLAGS) $(CFITSIO_LDFLAGS) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libcplui_la_LIBADD = $(LIBCPLCORE) $(LIBCFITSIO) $(LIBCEXT) -lm
libcplui_la_DEPENDENCIES = $(LIBCPLCORE)
