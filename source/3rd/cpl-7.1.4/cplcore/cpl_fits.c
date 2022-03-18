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

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_fits.h"
#include "cpl_io_fits.h"
#include "cpl_error_impl.h"
#include "cpl_memory.h"
#include "cpl_tools.h"

#include <string.h>
#include <fitsio.h>

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_fits   FITS related basic routines
 *
 * This module provides functions to get basic information on FITS files
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_fits.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/**
  @brief  Set the FITS I/O mode
  @param  mode The FITS I/O mode: CPL_FITS_STOP_CACHING, CPL_FITS_START_CACHING,
               CPL_FITS_RESTART_CACHING
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
  @see cpl_init() to control the FITS I/O mode when CPL is started

   Normally when a FITS file is processed with a CPL call the
   file is openened and closed during that call. However repeated calls on
   FITS data with many extensions causes the FITS headers to be parsed many
   times which can lead to a significant performance penalty.
   If instead this function is called with CPL_FITS_START_CACHING,
   CPL will use internal storage to keep the FITS files open between calls and
   only close them when the FITS I/O mode is changed (or cpl_end() is called).

   If a CPL function that creates a FITS file is called any previously opened
   handles to that file are closed. If a CPL function that appends to a FITS
   file is called any previously opened read-only handles to that file are
   closed. If a CPL function that reads from a FITS file is called any
   previously opened read/write-handle to that file is used for the read. Any
   additional concurrent reads causes the file to also be opened for reading.
   This means that there is also a performance gain when alternating between
   appending to and reading from the same file.
   This optimized I/O mode cannot be used if the CPL application accesses
   a FITS file without using CPL. An incomplete list of such access is:
   Calls to rename(), remove(), unlink(), fopen() and access via another
   process started with for example system() or popen().

   The caching of opened FITS files may be used in a multi-threaded environment
   to the extent that the underlying FITS library (CFITSIO) supports it.
   One implication of this
   is that multiple threads may only call CPL FITS saving functions on the same
   file using proper synchronization such as the OpenMP 'ordered' construct.
   CPL makes no attempt to verify that a CPL based application performs thread
   parallel FITS I/O correctly.

  The mode CPL_FITS_STOP_CACHING causes all cached filed to be closed.
  Beware that this can cause an I/O error for a file that has otherwise
  not been accessed for some time.

  Since CPL_FITS_STOP_CACHING closes all cached FITS files, the caller must
  ensure that this does not interfere with the concurrent use of those same
  files in another thread.

  The mode CPL_FITS_RESTART_CACHING has the same effect as a call with
  CPL_FITS_STOP_CACHING followed by a call with CPL_FITS_START_CACHING.

  The modes CPL_FITS_RESTART_CACHING and CPL_FITS_ONE may be combined.
  This causes all files cached by the calling thread to be closed. The
  caching remains active (for all threads), so subsequently opened files
  will be cached.

  CPL_FITS_RESTART_CACHING can be used after an external modification of
  a FITS file also cached by CPL, thus allowing the caching to work together
  with the above mentioned external access to the same FITS files.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if the mode is zero
  - CPL_ERROR_UNSUPPORTED_MODE if the mode is not supported
  - CPL_ERROR_INVALID_TYPE if mode is 1, e.g. due to a logical or (||)
      of the allowed options.
  - CPL_ERROR_BAD_FILE_FORMAT if the I/O caused by CPL_FITS_STOP_CACHING failed
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_fits_set_mode(cpl_fits_mode mode)
{

    if (mode == CPL_FITS_START_CACHING) {

        cpl_io_fits_init();

    } else if (mode == CPL_FITS_STOP_CACHING) {

        if (cpl_io_fits_end()) return cpl_error_set_where_();

    } else if ((mode & CPL_FITS_RESTART_CACHING) == CPL_FITS_RESTART_CACHING) {

        if (cpl_io_fits_close_tid((mode & CPL_FITS_ONE) ? CPL_IO_FITS_ONE
                                  : CPL_IO_FITS_ALL))
            return cpl_error_set_where_();

    } else if (mode == 0) {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    } else if (mode == 1) {
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    } else {
        return cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                      "mode=%d", (int)mode);
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief  Get the FITS I/O mode
  @return One of: CPL_FITS_STOP_CACHING, CPL_FITS_START_CACHING
  @see cpl_fits_set_mode()

 */
/*----------------------------------------------------------------------------*/
cpl_fits_mode cpl_fits_get_mode(void)
{

    return cpl_io_fits_is_enabled() ? CPL_FITS_START_CACHING
        : CPL_FITS_STOP_CACHING;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Get the number of extensions contained in a FITS file
  @param  filename    The file name
  @return The number of extensions or -1 in case of error
  @note For a valid fits file without extensions zero is returned

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if the input pointer is NULL
  - CPL_ERROR_FILE_IO If the FITS file could not be opened
  - CPL_ERROR_BAD_FILE_FORMAT if the input FITS file is otherwise invalid
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_fits_count_extensions(const char * filename)
{
    fitsfile * fptr;
    int        error = 0;
    int        next  = 0; /* CFITSIO supports only int */

    cpl_ensure(filename != NULL, CPL_ERROR_NULL_INPUT, -1);

    if (cpl_io_fits_open_diskfile(&fptr, filename, READONLY, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, fits_open_diskfile,
                                 "filename='%s'", filename);
        return -1;
    }

    if (fits_get_num_hdus(fptr, &next, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, error,
                                 fits_get_num_hdus, "filename='%s'", filename);
        error = 0; /* Reset so fits_close_file works */
        next = 0;
    }

    if (cpl_io_fits_close_file(fptr, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, fits_close_file,
                                 "filename='%s'", filename);
    }

    /* The number of HDUs includes the primary one which is not an extension */
    return (cpl_size)next - 1;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the place of a given extension in a FITS file
  @param    filename The file name
  @param    extname  The extension name
  @return   The extension number, 0 if not found or -1 on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_FILE_IO if the file is not FITS
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_fits_find_extension(const char * filename,
                                 const char * extname)
{
    fitsfile  * fptr;
    int         error = 0;
    cpl_size    ext_num    = 0;
    char        c_extname[FLEN_VALUE] = "";
    cpl_boolean has_card;

    cpl_ensure(filename != NULL, CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(extname  != NULL, CPL_ERROR_NULL_INPUT, -1);

    if (cpl_io_fits_open_diskfile(&fptr, filename, READONLY, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, fits_open_diskfile,
                                 "filename='%s', extname='%s'",
                                 filename, extname);
        return -1;
    }

    do {
        has_card = CPL_TRUE;
        if (!fits_movabs_hdu(fptr, 2 + ext_num, NULL, &error) &&
            fits_read_key_str(fptr, "EXTNAME", c_extname, NULL, &error) ==
            KEY_NO_EXIST) {
            /* EXTNAME card is absent, that is allowed */
            has_card = CPL_FALSE;
            error = 0; /* Reset so subsequent calls work */
        }
        if (!error) {
            ext_num++;
        }

    } while (!error && (!has_card || strncmp(extname, c_extname, FLEN_VALUE)));

    if (error) {
        ext_num = error == END_OF_FILE ? 0 : -1;
        error = 0; /* Reset so fits_close_file works */
    }

    if (cpl_io_fits_close_file(fptr, &error)) {
        (void)cpl_error_set_fits(CPL_ERROR_FILE_IO, error, fits_close_file,
                                 "filename='%s', extname='%s'",
                                 filename, extname);
        ext_num = -1;
    }

    return ext_num;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the number of extensions contained in a FITS file
  @param    filename    The file name
  @return   the number of extensions or -1 in case of error
  @see cpl_fits_count_extensions()
  @deprecated Replace this call with cpl_fits_count_extensions().

 */
/*----------------------------------------------------------------------------*/

int cpl_fits_get_nb_extensions(const char * filename)
{
    return (int)cpl_fits_count_extensions(filename);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the place of a given extension in a FITS file
  @param    filename    The file name
  @param    extname     The extension name
  @return   the extension place or -1 in case of error
  @see cpl_fits_find_extension
  @deprecated Replace this call with cpl_fits_find_extension().

 */
/*----------------------------------------------------------------------------*/

int cpl_fits_get_extension_nb(const char  *   filename,
                              const char  *   extname)
{
    return (int)cpl_fits_find_extension(filename, extname);
}

/**@}*/
