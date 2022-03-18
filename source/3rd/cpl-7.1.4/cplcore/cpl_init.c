/*
 * This file is part of the ESO Common Pipeline Library
 * Copyright (C) 2001-2017 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cpl_init.h"

#include "cpl_memory_impl.h"
#include "cpl_error_impl.h"
#include "cpl_msg.h"
#include "cpl_tools.h"
#include "cpl_fits.h"

#include <fitsio.h>

#include <float.h>

/* getenv() */
#include <stdlib.h>

/* strcmp() */
#include <string.h>

#ifdef CPL_WCS_INSTALLED    /* If WCS is installed */
/* Get WCSLIB version number */
#include <wcslib.h>
#endif                      /* End If WCS is installed */

#if defined CPL_FFTWF_INSTALLED || defined CPL_FFTW_INSTALLED
#include <fftw3.h>
#endif

#if !defined(CFITSIO_MAJOR) || !defined(CFITSIO_MINOR)
#  error "CFITSIO major and/or minor version are not set!"
#else
#  if (CFITSIO_MAJOR < 3) || ((CFITSIO_MAJOR == 3) && (CFITSIO_MINOR < 35))
#    error "CFITSIO version is not supported (too old)!"
#  endif
#endif


/**
 * @defgroup cpl_init Library Initialization
 *
 * The module provides the CPL library startup routine. The startup routine
 * initialises CPL internal data structures. For this reason, any application
 * using functions from the CPL libraries @b must call the startup routine
 * prior to calling any other CPL function.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_init.h>
 * @endcode
 */

/**@{*/

/**
 * @brief
 *   Initialise the CPL core library.
 *
 * @param self  @em CPL_INIT_DEFAULT is the only supported value
 * @return Nothing.
 * @note The function must be called once before any other CPL function.
 * @see cpl_fits_set_mode()
 *
 * This function sets up the library internal subsystems, which other 
 * CPL functions expect to be in a defined state. In particular, the CPL
 * memory management and the CPL messaging systems are initialised by 
 * this function call.
 *
 * One of the internal subsystems of CPL handles memory allocation.
 * The default CPL memory mode is defined during the build procedure,
 * this default can be changed during the call to cpl_init() via the
 * environment variable @em CPL_MEMORY_MODE. The valid values are
 * 0: Use the default system functions for memory handling
 * 1: Exit if a memory-allocation fails, provide checking for memory leaks,
 *    limited reporting of memory allocation and limited protection on
 *    deallocation of invalid pointers.
 * 2: Exit if a memory-allocation fails, provide checking for memory leaks,
 *    extended reporting of memory allocation and protection on deallocation
 *    of invalid pointers.
 * Any other value (including NULL) will leave the default memory mode
 * unchanged.
 * 
 * This function also reads the environment variable @em CPL_IO_MODE.
 * Iff set to 1, cpl_fits_set_mode() is called with CPL_FITS_START_CACHING.
 *
 * Possible #_cpl_error_code_ set in this function:
 * - CPL_ERROR_INCOMPATIBLE_INPUT if there is an inconsistency between the run-
 *   time and compile-time versions of a library that CPL depends on internally,
 *   e.g. CFITSIO. This error may occur with dynamic linking. If it does occur,
 *   the use of CPL may lead to unexpected behaviour.
 */

void
cpl_init(unsigned self)
{

    int memory_mode = CPL_XMEMORY_MODE; /* Default from configure */
    const char * memory_mode_string = getenv("CPL_MEMORY_MODE");
    const char * io_fits_mode_string = getenv("CPL_IO_MODE");
    const cpl_boolean use_io_fits = io_fits_mode_string != NULL &&
        strcmp("1", io_fits_mode_string) == 0;


    if (memory_mode_string != NULL) {
        if (strcmp("0", memory_mode_string) == 0) {
            memory_mode = 0;
        } else if (strcmp("1", memory_mode_string) == 0) {
            memory_mode = 1;
        } else if (strcmp("2", memory_mode_string) == 0) {
            memory_mode = 2;
        } /* else: Ignore the environment variable */
    }

    cpl_memory_init(memory_mode);
    cpl_msg_init();

    if (self != CPL_INIT_DEFAULT) {
        /* Avoid unused variable warning */
        cpl_msg_warning(cpl_func, "Illegal input ignored");
    }

    float _cfitsio_version;
    const int cfitsio_version_min = (int)(1000. * CPL_CFITSIO_VERSION + 0.5);
    const int cfitsio_version = (int)(1000. *
            fits_get_version(&_cfitsio_version) + 0.5);

    if (cfitsio_version < cfitsio_version_min) {
        (void)cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                     "Run-time version %.3f of CFITSIO "
                                     "is not supported. Minimum required "
                                     "version  is %.3f", _cfitsio_version,
                                     CPL_CFITSIO_VERSION);
    }

#ifdef HAVE_LIBPTHREAD
    cpl_error_init_locks();
#endif

    if (use_io_fits) {
        cpl_fits_set_mode(CPL_FITS_START_CACHING);
    }

    return;
}




/*----------------------------------------------------------------------------*/
/**
   @brief   Create a string of version numbers of CPL and its libraries
   @param   self  CPL_DESCRIPTION_DEFAULT
   @return  A pointer to a constant character array
*/
/*----------------------------------------------------------------------------*/
const char * cpl_get_description(unsigned self)
{

    /* At a later stage decription_mode may be used to select what
       version information to return, e.g. CPL only, 3rd party libraries
       only, or both */

#ifdef CPL_WCS_INSTALLED
#ifdef WCSLIB_VERSION
#define CPL_WCS_APPEND ", WCSLIB = " CPL_STRINGIFY(WCSLIB_VERSION)
#else
#define CPL_WCS_APPEND ", WCSLIB"
#endif
#else
#define CPL_WCS_APPEND " (WCSLIB unavailable)"
#endif

#if defined CPL_FFTW_INSTALLED && defined CPL_FFTWF_INSTALLED
#if defined CPL_FFTW_VERSION && defined CPL_FFTWF_VERSION
#define CPL_FFTW_APPEND ", FFTW (normal precision) = " CPL_FFTW_VERSION \
                        ", FFTW (single precision) = " CPL_FFTWF_VERSION
#else
#define CPL_FFTW_APPEND ", FFTW (normal and single precision)"
#endif
#elif defined CPL_FFTW_INSTALLED
#if defined CPL_FFTW_VERSION
#define CPL_FFTW_APPEND ", FFTW (normal precision) = " CPL_FFTW_VERSION
#else
#define CPL_FFTW_APPEND ", FFTW (normal precision)"
#endif
#elif defined CPL_FFTWF_INSTALLED
#if defined CPL_FFTWF_VERSION
#define CPL_FFTW_APPEND ", FFTW (single precision) = " CPL_FFTWF_VERSION
#else
#define CPL_FFTW_APPEND ", FFTW (single precision)"
#endif
#else
#define CPL_FFTW_APPEND " (FFTW unavailable)"
#endif

#ifdef CPL_ADD_FLOPS
#define CPL_FLOPS_APPEND                        \
    ", CPL FLOP counting is available"
#else
#define CPL_FLOPS_APPEND                                                \
    ", CPL FLOP counting is unavailable, enable with -DCPL_ADD_FLOPS"
#endif

#ifdef _OPENMP
#define CPL_OPENMP_APPEND ", OPENMP = " CPL_STRINGIFY(_OPENMP)
#else
#define CPL_OPENMP_APPEND ""
#endif

#ifdef CPL_IO_FITS_MAX_OPEN
#define CPL_IO_FITS_APPEND ", CPL_IO_FITS_OPEN = " \
        CPL_STRINGIFY(CPL_IO_FITS_MAX_OPEN)
#else
#define CPL_IO_FITS_APPEND ""
#endif


    if (self != CPL_DESCRIPTION_DEFAULT) {
        /* Avoid unused variable warning */
        cpl_msg_warning(cpl_func, "Illegal input ignored");
    }

#ifdef CFITSIO_VERSION
    return "CPL = " VERSION ", CFITSIO = " CPL_STRINGIFY(CFITSIO_VERSION)
        CPL_WCS_APPEND CPL_FFTW_APPEND CPL_FLOPS_APPEND CPL_OPENMP_APPEND
        CPL_IO_FITS_APPEND;
#else

    /* FIXME: Verify that 3.03 introduced CFITSIO_VERSION */
    return "CPL = " VERSION ", CFITSIO less than 3.03"
        CPL_WCS_APPEND CPL_FFTW_APPEND CPL_FLOPS_APPEND CPL_OPENMP_APPEND
        CPL_IO_FITS_APPEND;
#endif

}


/**
 * @brief
 *   Stop the internal subsystems of CPL.
 *
 * @return Nothing.
 * @note   Currently, the behaviour of any CPL function becomes
 * undefined after this function is called.
 *
 * This function must be called after any other CPL function
 * is called.
 * 
 */

void
cpl_end(void)
{

    (void)cpl_fits_set_mode(CPL_FITS_STOP_CACHING);

#ifdef CPL_FFTWF_INSTALLED
    fftwf_cleanup();
#endif
#ifdef CPL_FFTW_INSTALLED
    fftw_cleanup();
#endif

    cpl_msg_stop();

    return;

}


/**@}*/
