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

AM_CPPFLAGS = $(CPLCORE_INCLUDES) $(CX_INCLUDES) $(CFITSIO_INCLUDES) \
	          $(CPL_CFLAGS)
AM_LDFLAGS = $(CFITSIO_LDFLAGS)

LDADD = $(LIBCPLCORE) $(LIBCEXT) $(LIBCFITSIO) -lm

EXTRA_DIST = cpl_filter_body.h

check_PROGRAMS = cpl_array-test \
                 cpl_type-test \
                 cpl_tools-test \
                 cpl_image_io-test \
                 cpl_image_basic-test \
                 cpl_image_iqe-test \
                 cpl_image_bpm-test \
                 cpl_image_resample-test \
                 cpl_filter-test \
                 cpl_fits-test \
                 cpl_stats-test \
                 cpl_math-test \
                 cpl_image_filter-test \
                 cpl_image_gen-test \
                 cpl_imagelist_io-test \
                 cpl_imagelist_basic-test \
                 cpl_io_fits-test \
                 cpl_mask-test \
                 cpl_table-test \
                 cpl_test-test \
                 cpl_test_init-test \
                 cpl_matrix-test \
                 cpl_msg-test \
                 cpl_memory-test \
                 cpl_plot-test \
                 cpl_polynomial-test \
                 cpl_error-test \
                 cpl_errorstate-test \
                 cpl_vector-test \
                 cpl_bivector-test \
                 cpl_property-test \
                 cpl_propertylist-test \
                 cpl_median-test

cpl_array_test_SOURCES = cpl_array-test.c
cpl_image_io_test_SOURCES = cpl_image_io-test.c
cpl_image_basic_test_SOURCES = cpl_image_basic-test.c
cpl_image_iqe_test_SOURCES = cpl_image_iqe-test.c
cpl_image_bpm_test_SOURCES = cpl_image_bpm-test.c
cpl_image_resample_test_SOURCES = cpl_image_resample-test.c
cpl_filter_test_SOURCES = cpl_filter-test.c
cpl_fits_test_SOURCES = cpl_fits-test.c
cpl_stats_test_SOURCES = cpl_stats-test.c
cpl_math_test_SOURCES = cpl_math-test.c
cpl_image_gen_test_SOURCES = cpl_image_gen-test.c
cpl_image_filter_test_SOURCES = cpl_image_filter-test.c
cpl_imagelist_io_test_SOURCES = cpl_imagelist_io-test.c
cpl_imagelist_basic_test_SOURCES = cpl_imagelist_basic-test.c
cpl_io_fits_test_SOURCES = cpl_io_fits-test.c
cpl_mask_test_SOURCES = cpl_mask-test.c
cpl_table_test_SOURCES = cpl_table-test.c
cpl_test_test_SOURCES = cpl_test-test.c
cpl_test_init_test_SOURCES = cpl_test_init-test.c
cpl_type_test_SOURCES = cpl_type-test.c
cpl_tools_test_SOURCES = cpl_tools-test.c
cpl_matrix_test_SOURCES = cpl_matrix-test.c
cpl_msg_test_SOURCES = cpl_msg-test.c
cpl_memory_test_SOURCES = cpl_memory-test.c
cpl_plot_test_SOURCES = cpl_plot-test.c
cpl_polynomial_test_SOURCES = cpl_polynomial-test.c
cpl_error_test_SOURCES = cpl_error-test.c
cpl_errorstate_test_SOURCES = cpl_errorstate-test.c
cpl_vector_test_SOURCES = cpl_vector-test.c
cpl_bivector_test_SOURCES = cpl_bivector-test.c
cpl_property_test_SOURCES = cpl_property-test.c
cpl_propertylist_test_SOURCES = cpl_propertylist-test.c
cpl_median_test_SOURCES = cpl_median-test.c

# Be sure to reexport important environment variables.
TESTS_ENVIRONMENT = MAKE="$(MAKE)" CC="$(CC)" CFLAGS="$(CFLAGS)" \
	CPPFLAGS="$(CPPFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS)" \
	LIBS="$(LIBS)" LN_S="$(LN_S)" NM="$(NM)" RANLIB="$(RANLIB)" \
	OBJEXT="$(OBJEXT)" EXEEXT="$(EXEEXT)"


TESTS = cpl_array-test \
        cpl_image_io-test \
        cpl_image_basic-test \
        cpl_image_iqe-test \
        cpl_image_bpm-test \
        cpl_image_resample-test \
        cpl_filter-test \
        cpl_fits-test \
        cpl_stats-test \
        cpl_math-test \
        cpl_image_gen-test \
        cpl_image_filter-test \
        cpl_imagelist_io-test \
        cpl_imagelist_basic-test \
        cpl_io_fits-test \
        cpl_mask-test \
        cpl_plot-test \
        cpl_polynomial-test \
        cpl_error-test \
        cpl_errorstate-test \
        cpl_table-test \
        cpl_test-test \
        cpl_test_init-test \
        cpl_type-test \
        cpl_tools-test \
        cpl_matrix-test \
        cpl_msg-test \
        cpl_memory-test \
        cpl_vector-test \
        cpl_bivector-test \
        cpl_property-test \
        cpl_propertylist-test \
        cpl_median-test

# We need to remove any files that the above tests created.
clean-local:
	$(RM) *.fits *.tfits *.log
