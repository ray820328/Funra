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

ACLOCAL_AMFLAGS = -I m4

DISTCLEANFILES = *~


SUBDIRS = cext tests

DOXYGEN_SUBDIRS =


EXTRA_DIST = BUGS Doxyfile.in cext.pc.in m4/eso.m4 \
    admin/doxygen.am


pkgconfig_DATA = cext.pc


if MAINTAINER_MODE

MAINTAINERCLEANFILES = $(top_srcdir)/Makefile.in $(top_srcdir)/aclocal.m4 \
	$(top_srcdir)/config.h.in $(top_srcdir)/configure config.log \
	$(top_builddir)/config.status

clean-local: clean-doxygen

dist-hook: doxygen
	@if test -d $(top_builddir)/html; then \
	  echo "cp -pr $(top_builddir)/html $(distdir)"; \
	  cp -pr $(top_builddir)/html $(distdir); \
	fi
	find $(distdir) -type d ! -perm -222 -exec chmod u+w {} \; -o \
	-type f ! -perm -222 -exec chmod u+w {} \; || chmod -R u+w $(distdir)

else

clean-local:

endif

uninstall-local: uninstall-doxygen

include $(top_srcdir)/admin/doxygen.am
