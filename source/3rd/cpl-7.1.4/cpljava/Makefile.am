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

AM_CPPFLAGS = $(CPLCORE_INCLUDES) $(CPLUI_INCLUDES) $(CPLDFS_INCLUDES) \
              $(CX_INCLUDES) $(JAVA_INCLUDES) $(INCLTDL)

GASGANO_NATIVE_INCLUDES = org_eso_cpl_jni_CPLControl.h \
	org_eso_cpl_jni_JNIParameterImp.h \
	org_eso_cpl_jni_JNIRecipe.h \
	org_eso_cpl_jni_LibraryLoader.h \
	org_eso_cpl_jni_PluginLibrary.h

lib_LTLIBRARIES = libcplgasgano.la

noinst_HEADERS = $(GASGANO_NATIVE_INCLUDES)

libcplgasgano_la_SOURCES = cpl_gasgano.c

libcplgasgano_la_LDFLAGS = $(CX_LDFLAGS) $(GASGANO_SHREXT) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libcplgasgano_la_LIBADD =  $(LIBCPLDFS) $(LIBCPLUI) $(LIBCPLCORE) $(LIBCEXT) $(LIBLTDL)
libcplgasgano_la_DEPENDENCIES = $(LIBCPLDFS) $(LIBCPLUI) $(LIBCPLCORE) $(LIBLTDL)

cpl_gasgano.c: $(GASGANO_NATIVE_INCLUDES)
