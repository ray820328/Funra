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


# Place optional 3rd party components last since those locations may contain
# obsolete and therefore unwanted CFITSIO installations

AM_CPPFLAGS = -DCX_LOG_DOMAIN=\"CplDrs\" \
              $(CPLDRS_INCLUDES) $(CPLCORE_INCLUDES) $(CX_INCLUDES) \
	          $(CFITSIO_INCLUDES) $(WCS_INCLUDES) $(FFTW_INCLUDES) \
	          $(FFTWF_INCLUDES)

include_HEADERS =   cpl_apertures.h \
                    cpl_apertures_img.h \
                    cpl_geom_img.h \
                    cpl_detector.h \
                    cpl_phys_const.h \
                    cpl_photom.h \
                    cpl_fit.h \
                    cpl_ppm.h \
                    cpl_wcs.h \
                    cpl_wlcalib.h \
                    cpl_fft.h

noinst_HEADERS =    cpl_geom_img_body.h \
                    cpl_fit_body.h \
                    cpl_fft_body.h \
                    cpl_wlcalib_impl.h \
                    cpl_detector_body.h

lib_LTLIBRARIES = libcpldrs.la

libcpldrs_la_SOURCES =  cpl_apertures.c \
                        cpl_detector.c \
                        cpl_geom_img.c \
                        cpl_photom.c \
                        cpl_fit.c \
                        cpl_ppm.c \
                        cpl_wcs.c \
                        cpl_wlcalib.c \
                        cpl_fft.c

# Place optional 3rd party components last since those locations may contain
# obsolete and therefore unwanted CFITSIO installations
libcpldrs_la_LDFLAGS = $(CX_LDFLAGS) $(CFITSIO_LDFLAGS) $(FFTW_LDFLAGS) $(FFTWF_LDFLAGS) $(WCS_LDFLAGS) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libcpldrs_la_LIBADD = $(LIBCPLCORE) $(LIBCFITSIO) $(LIBWCS) $(LIBCEXT) $(LIBFFTW) $(LIBFFTWF) -lm
libcpldrs_la_DEPENDENCIES = $(LIBCPLCORE)
