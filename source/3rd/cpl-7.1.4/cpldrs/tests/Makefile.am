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

AM_CPPFLAGS = $(CPLDRS_INCLUDES) $(CPLCORE_INCLUDES) $(CX_INCLUDES) \
	          $(CFITSIO_INCLUDES) $(WCS_INCLUDES) \
	          $(CPL_CFLAGS)

LDADD = $(LIBCPLDRS) $(LIBCPLCORE) $(LIBCEXT) $(LIBWCS) -lm
AM_LDFLAGS = $(WCS_LDFLAGS)

check_PROGRAMS = cpl_fit-test \
                 cpl_apertures-test \
                 cpl_geom_img-test \
                 cpl_detector-test \
                 cpl_photom-test \
                 cpl_wcs-test \
                 cpl_wlcalib-test \
                 cpl_ppm-test \
                 cpl_fft-test

cpl_fit_test_SOURCES = cpl_fit-test.c
cpl_apertures_test_SOURCES = cpl_apertures-test.c
cpl_detector_test_SOURCES = cpl_detector-test.c
cpl_geom_img_test_SOURCES = cpl_geom_img-test.c
cpl_photom_test_SOURCES = cpl_photom-test.c
cpl_wcs_test_SOURCES = cpl_wcs-test.c
cpl_wlcalib_test_SOURCES = cpl_wlcalib-test.c
cpl_ppm_test_SOURCES = cpl_ppm-test.c
cpl_fft_test_SOURCES = cpl_fft-test.c

# Be sure to reexport important environment variables.
TESTS_ENVIRONMENT = MAKE="$(MAKE)" CC="$(CC)" CFLAGS="$(CFLAGS)" \
	CPPFLAGS="$(CPPFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS)" \
	LIBS="$(LIBS)" LN_S="$(LN_S)" NM="$(NM)" RANLIB="$(RANLIB)" \
	OBJEXT="$(OBJEXT)" EXEEXT="$(EXEEXT)"


TESTS = cpl_fit-test \
        cpl_apertures-test \
        cpl_geom_img-test \
        cpl_detector-test \
        cpl_photom-test \
        cpl_wcs-test \
        cpl_wlcalib-test \
        cpl_ppm-test \
        cpl_fft-test

XFAIL_TESTS =

# We need to remove any files that the above tests created.
clean-local:
	$(RM) planck1.txt planck2.txt *.log
