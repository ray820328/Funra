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

DISTCLEANFILES = cpl_version.h.in cpl_version.h cpl_func.h.in cpl_func.h *~

SUBDIRS = tests


if MAINTAINER_MODE

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

endif


## WCS_INCLUDES to get WCSLIB version number in cpl_init()
# Place optional 3rd party components last since those locations may contain
# obsolete and therefore unwanted CFITSIO installations

AM_CPPFLAGS = -DCX_LOG_DOMAIN=\"CplCore\" \
	          -I\$(top_srcdir)/cpldrs $(CX_INCLUDES) $(CFITSIO_INCLUDES) \
	          $(WCS_INCLUDES) $(FFTW_INCLUDES) $(FFTWF_INCLUDES) \
	          $(CPL_CFLAGS)

EXTRA_DIST = cpl_version.h.top cpl_version.h.bot cpl_func.h.top cpl_func.h.bot \
             cpl_filter_median.c

BUILT_SOURCES = cpl_version.h cpl_func.h

include_HEADERS = cpl_bivector.h \
              cpl_error.h \
              cpl_errorstate.h \
              cpl_fits.h \
              cpl_filter.h \
              cpl_image.h \
              cpl_image_basic.h \
              cpl_image_iqe.h \
              cpl_image_bpm.h \
              cpl_image_resample.h \
              cpl_image_io.h \
              cpl_stats.h \
              cpl_image_stats.h \
              cpl_image_filter.h \
              cpl_image_gen.h \
              cpl_imagelist.h \
              cpl_imagelist_io.h \
              cpl_imagelist_basic.h \
              cpl_init.h \
              cpl_io.h \
              cpl_macros.h \
              cpl_mask.h \
              cpl_matrix.h \
              cpl_memory.h \
              cpl_msg.h \
              cpl_plot.h \
              cpl_polynomial.h \
              cpl_property.h \
              cpl_propertylist.h \
              cpl_array.h \
              cpl_table.h \
              cpl_type.h \
              cpl_math_const.h \
              cpl_test.h \
              cpl_vector.h


nodist_include_HEADERS = cpl_version.h cpl_func.h

noinst_HEADERS = cpl_array_impl.h cpl_column.h cpl_error_impl.h \
	cpl_image_basic_body.h cpl_image_defs.h cpl_image_filter_body.h \
	cpl_image_gen_body.h cpl_image_io_body.h cpl_image_resample_body.h \
	cpl_image_stats_body.h cpl_imagelist_basic_body.h cpl_imagelist_defs.h \
	cpl_mask_defs.h cpl_mask_impl.h cpl_memory_impl.h cpl_propertylist_impl.h \
	cpl_stats_body.h cpl_tools.h cpl_tools_body.h cpl_type_impl.h \
	cpl_image_bpm_impl.h \
	cpl_xmemory.h cpl_image_filter_impl.h cpl_mask_body.h cpl_mask_binary.h \
	cpl_image_bpm_body.h cpl_io_fits.h cpl_cfitsio.h cpl_polynomial_impl.h \
	cpl_vector_impl.h cpl_vector_fit_impl.h cpl_matrix_impl.h \
	cpl_stats_defs.h cpl_stats_impl.h cpl_column_body.h cpl_property_impl.h \
	cpl_image_io_impl.h cpl_image_basic_impl.h cpl_image_fft_impl.h cpl_mpfit.h

lib_LTLIBRARIES = libcplcore.la

## The below source is listed alphabetically, except for
## cpl_column.c and cpl_image_gen.c. This allows compilation
## of the other source code to be compiled by clang 2.8 && Sun Studio 12.1

libcplcore_la_SOURCES = cpl_array.c \
			cpl_bivector.c \
			cpl_error.c \
			cpl_errorstate.c \
			cpl_fits.c \
			cpl_io_fits.c \
			cpl_cfitsio.c \
			cpl_image_basic.c \
			cpl_image_bpm.c \
			cpl_image_filter.c \
			cpl_image_io.c \
			cpl_image_iqe.c \
			cpl_image_resample.c \
			cpl_image_stats.c \
			cpl_image_fft.c \
			cpl_imagelist_basic.c \
			cpl_imagelist_io.c \
			cpl_init.c \
			cpl_mask.c \
			cpl_matrix.c \
			cpl_memory.c \
			cpl_msg.c \
			cpl_plot.c \
			cpl_polynomial.c \
			cpl_property.c \
			cpl_propertylist.c \
			cpl_stats.c \
			cpl_table.c \
			cpl_test.c \
			cpl_tools.c \
			cpl_type.c \
			cpl_vector.c \
			cpl_version.c \
			cpl_xmemory.c \
			cpl_image_gen.c \
			cpl_column.c \
			cpl_mpfit.c

# Place optional 3rd party components last since those locations may contain
# obsolete and therefore unwanted CFITSIO installations
libcplcore_la_LDFLAGS = $(CX_LDFLAGS) $(CFITSIO_LDFLAGS) $(FFTW_LDFLAGS) $(FFTWF_LDFLAGS) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libcplcore_la_LIBADD =  $(LIBCEXT) $(LIBCFITSIO) $(LIBFFTW) $(LIBFFTWF) -lm
libcplcore_la_DEPENDENCIES =


cpl_version.h: $(srcdir)/cpl_version.h.top cpl_version.h.in $(srcdir)/cpl_version.h.bot
	cat $(srcdir)/cpl_version.h.top cpl_version.h.in \
	    $(srcdir)/cpl_version.h.bot > cpl_version.h;

cpl_func.h: $(srcdir)/cpl_func.h.top cpl_func.h.in $(srcdir)/cpl_func.h.bot
	cat $(srcdir)/cpl_func.h.top cpl_func.h.in \
	    $(srcdir)/cpl_func.h.bot > cpl_func.h;
