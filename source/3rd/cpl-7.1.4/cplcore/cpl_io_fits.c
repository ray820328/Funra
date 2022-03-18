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

#include "cpl_io_fits.h"

#include <cpl_memory.h>
#include <cpl_error_impl.h>

#include <cxlist.h>

#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#endif

/* The below doxygen has been inactivated by removing the '**' comment. */

/*----------------------------------------------------------------------------*/
/*
 * @defgroup cpl_io_fits   Optimize open and close of FITS files
 *
 * The CPL API for FITS I/O passes only the FITS file name, and per default
 * opens and closes each file for each I/O operation. Since the FITS standard
 * does not allow random access to a given extension, the open/close approach
 * causes the writing of a file with N extensions to have complexity O(N^2).
 * The same is true for reading all N extensions.
 *
 * The complexity of those operations can be reduced to the expected O(N) by
 * keeping the FITS files open between operations. This is done with 
 * static (thread-shared) storage of the relevant data.
 * 
 * In a multi-threaded environment it is assumed that if one thread enters
 * a CPL FITS save function for a given file, then there are no concurrent
 * threads inside a CPL FITS I/O function for the same file. Consequently, in a
 * multi-threaded environment it is assumed that if one thread enters a CPL FITS
 * load function for a given file, then there are no concurrent threads inside a
 * CPL FITS save function.
 * 
 * This means that it is safe to let different threads take turns using the
 * same read/write handle (for reading and/or writing).
 * 
 * A handle for read-only is only used by the creating thread, this allows
 * different threads to read from different parts of the same file.
 * 
 * The unit tests in cplcore/tests/cpl_io_fits-test.c provide examples of this.
 * 
 * @par Synopsis:
 * @code
 *   #include "cpl_io_fits.h"
 * @endcode
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                               Private types
 -----------------------------------------------------------------------------*/

typedef struct cpl_fitsfile_t {
    char      * name;
    fitsfile  * fptr;
    int         iomode; /* CFITSIO currently defines: READONLY, READWRITE */
    cpl_boolean has_stat; /* Set to true iff stat() can be & was called OK. */
                          /* When false, the below members are undefined */
#ifdef CPL_HAVE_STAT
    dev_t      st_dev; /* ID of device containing file */
    ino_t      st_ino; /* inode number */
#endif
    int        tid; /* Thread id. It must be matched for read-reuse.
                       If must also be matched if a file must be closed
                       prematurely because there are too many open files.
                       If matched for write-reuse the thread id is therefore
                       modified to that of the reuser. */
    cpl_boolean writing; /* CPL_TRUE iff a file is used for writing. If a
                            file opened for writing is reused for reading,
                            then this flag is set to false, indicating that
                            subsequent reuse for reading must match the
                            thread id. */

} cpl_fitsfile_t;

/*-----------------------------------------------------------------------------
                        Private variables
 -----------------------------------------------------------------------------*/

/* The maximum number of open FITS-files */
static cpl_size cpl_io_max_open = (CPL_IO_FITS_MAX_OPEN);
static cpl_size cpl_nfitsfiles = 0;    /* The number of open FITS-files */
static cx_list * cpl_fitslist  = NULL; /* The list of open, cached FITS-files */

/*-----------------------------------------------------------------------------
                                   Private functions
 -----------------------------------------------------------------------------*/

#ifdef CPL_IO_FITS
static cpl_boolean cpl_io_fits_find_fptr(cx_list_iterator *, const char *,
                                         const int *, const struct stat *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;
#endif

static fitsfile * cpl_io_fits_unset_fptr(const char *, const int *);
static fitsfile * cpl_io_fits_reuse_fptr(const char *, int, cpl_boolean)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;
static const char * cpl_io_fits_find_name(const fitsfile *, int *)
    CPL_ATTR_NONNULL;
static void cpl_io_fits_set(fitsfile *, const char *, int, cpl_boolean)
    CPL_ATTR_NONNULL;

static cpl_fitsfile_t * cpl_io_fits_unset_tid(int);
static int cpl_io_fits_free(cpl_fitsfile_t *, cpl_boolean, int *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(3)))
#endif
    ;

/*-----------------------------------------------------------------------------
                              Function definitions
 -----------------------------------------------------------------------------*/

/**
 * @internal
 * @brief Initialize the caching of FITS-files
 * @return void
 * @see cpl_io_fits_end()
 * @note If the caching is already active, nothing happens
 */
void cpl_io_fits_init(void)
{
#if defined HAVE_SYSCONF && defined _SC_OPEN_MAX
    const long open_max = sysconf(_SC_OPEN_MAX);

    if (0 <= open_max && open_max/2 < (CPL_IO_FITS_MAX_OPEN))
        cpl_io_max_open = (cpl_size)(open_max/2);
#endif

#ifdef CPL_IO_FITS_DEBUG
    cpl_msg_debug(cpl_func, cpl_fitslist == NULL
                  ? "Initializing, max file pointers: %" CPL_SIZE_FORMAT " <= "
                  CPL_STRINGIFY(CPL_IO_FITS_MAX_OPEN)
                  : "Already initialized, max file pointers: %"
                  CPL_SIZE_FORMAT " <= "
                  CPL_STRINGIFY(CPL_IO_FITS_MAX_OPEN), cpl_io_max_open);
#endif

#ifdef CPL_IO_FITS
#ifdef _OPENMP
#pragma omp critical(cpl_io_fits)
#endif
    {
        if (cpl_fitslist == NULL) {
            cpl_fitslist = cx_list_new();
        }
    }
#endif
}

/**
 * @internal
 * @brief  Close all open FITS files
 * @return CPL_ERROR_NONE or the relevant #_cpl_error_code_ on error
 * @see cpl_io_fits_init()
 * @note Must be called before program termination, after it is called
 *       no other functions from this module may be called
 * 
 */
cpl_error_code cpl_io_fits_end(void)
{
    const cpl_error_code error = cpl_io_fits_close_tid(CPL_IO_FITS_ALL);

#ifdef CPL_IO_FITS_DEBUG
    cpl_msg_debug(cpl_func, "Finished: " CPL_STRINGIFY(CPL_IO_FITS_MAX_OPEN));
#endif

#ifdef CPL_IO_FITS

#ifdef _OPENMP
#pragma omp critical(cpl_io_fits)
#endif
    {
        if (cpl_fitslist != NULL) {
            cx_list_delete(cpl_fitslist);
            cpl_fitslist  = NULL;
        }
    }
#endif

    return error ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Close all files in use by the specified thread(s) (current or all)
  @param mode CPL_IO_FITS_ALL (all threads) or CPL_IO_FITS_ONE (current thread)
  @return Zero on success or else the CFITSIO status
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_io_fits_close_tid(cpl_boolean mode)
{
    int status = 0;

    if (cpl_fitslist != NULL) {
        const int tid = mode == CPL_IO_FITS_ONE ? omp_get_thread_num() : -1;
        cpl_fitsfile_t * oldest;

        do {

#ifdef _OPENMP
#pragma omp critical(cpl_io_fits)
#endif
            oldest = cpl_io_fits_unset_tid(tid);

            /* If a matching file is found, close it */
        } while (oldest != NULL &&
                 !cpl_io_fits_free(oldest, CPL_FALSE, &status));
    }

    return status ? cpl_error_set_where_() : CPL_ERROR_NONE;
}


/**
 * @internal
 * @brief  Return true iff the I/O FITS optimized mode is enabled
 * @return CPL_TRUE iff the I/O FITS optimized mode is enabled
 * @see cpl_io_fits_init()
 * 
 */
cpl_boolean cpl_io_fits_is_enabled(void)
{
    return cpl_fitslist != NULL ? CPL_TRUE : CPL_FALSE;
}


/**
 * @internal
 * @brief Open a fits file and destroy any preexisting file
 * @param pfptr    CFITSIO file pointer pointer to file
 * @param filename Name of FITS file to open
 * @param status   Pointer to CFITSIO error status
 * @return         The CFITSIO error status
 * @see fits_create_file()
 * @note Since the underlying CFITSIO call supports meta-characters _all_
 * currently open files are closed prior to opening this one.
 *
 */
int cpl_io_fits_create_file(fitsfile **pfptr, const char *filename, int *status)
{
    if (*status == 0) { /* Duplicate CFITSIO behaviour */

        /* The caller comes from a cpl_*_save(), so we are free
           to assume that no other thread is inside a cpl I/O function
           concerning the same file. We can therefore unset and close all
           file pointers open for that filename. */

        while ((*pfptr = cpl_io_fits_unset_fptr(filename, NULL)) != NULL &&
               !fits_close_file(*pfptr, status)) {
#ifdef _OPENMP
#pragma omp atomic
#endif
            cpl_nfitsfiles--;
#ifdef CPL_IO_FITS_DEBUG
            cpl_msg_debug(cpl_func, "Closed file: %s (%p) (%d)", filename,
                          (const void*)*pfptr, (int)cpl_nfitsfiles);
#endif
        }

        if (*pfptr == NULL) {
            if (cpl_fitslist != NULL) {
                const int tid = omp_get_thread_num();
                cpl_fitsfile_t * oldest = NULL;
#ifdef _OPENMP
                /* Comparison critical with cpl_nfitsfiles increment */
#pragma omp critical(cpl_io_fits)
#endif
                {
                    /* Need to open a file. Incerement prior to actual open,
                       in order to avoid the case where a number of synchronized
                       threads first verify that the number of files is (just)
                       below the limit and then the all try to open, thus 
                       exceeding the limit */

                    cpl_nfitsfiles++;
                    if (cpl_nfitsfiles > cpl_io_max_open) {
                        /* First need to close a file - find it first */
                        oldest = cpl_io_fits_unset_tid(tid);
                    }
                }
                if (cpl_io_fits_free(oldest, CPL_FALSE, status)) {
#ifdef _OPENMP
#pragma omp atomic
#endif
                    cpl_nfitsfiles--; /* The close failed, so no open */
                    return *status;
                }
            }
        }

        if (fits_create_file(pfptr, filename, status)) {
            (void)cpl_error_set_fits(CPL_ERROR_FILE_NOT_CREATED, *status,
                                     fits_create_file, "filename='%s'",
                                     filename);
#ifdef _OPENMP
#pragma omp atomic
#endif
            cpl_nfitsfiles--; /* The open failed */
        } else {

#ifdef CPL_IO_FITS_DEBUG
            cpl_msg_debug(cpl_func, "Opened file for writing: %s (%p) (%d)",
                          filename, (const void*)*pfptr,
                          (int)cpl_nfitsfiles);
#endif

            /* FIXME: Assume READWRITE */
            cpl_io_fits_set(*pfptr, filename, READWRITE, CPL_TRUE);
        }
    }

    return *status;
}

/**
 * @internal
 * @brief Try to reuse an already existing CFITSIO file pointer or reopen
 * @param pfptr    CFITSIO file pointer pointer to file
 * @param filename Name of FITS file to open
 * @param iomode   The CFITSIO iomode
 * @param status   Pointer to CFITSIO error status
 * @return         The CFITSIO error status
 * @see fits_open_diskfile()
 * @note Since this call may not actually open the file, the caller must
 *       use fits_movabs_hdu() and not fits_movrel_hdu().
 *
 */
int cpl_io_fits_open_diskfile(fitsfile **pfptr, const char * filename,
                              int iomode, int *status)
{

    if (*status == 0) { /* Duplicate CFITSIO behaviour */
        const int rmiomode = iomode == READONLY ? READWRITE : READONLY;

        if (iomode == READONLY) {
            /* If the caller comes from a cpl_*_load() then we are free to
               assume that no other thread is inside a cpl_*_save()
               concerning the same file. If a writer file pointer exists
               we can therefore unset and reuse it for reading. */

            *pfptr = cpl_io_fits_reuse_fptr(filename, rmiomode, CPL_FALSE);
            if (*pfptr != NULL) {
                iomode = rmiomode; /* Reuse a READWRITE handle for reading */
                /* A given file has at most one writer handle */
                /* assert( cpl_io_fits_unset_fptr(filename, &rmiomode) ==
                           NULL); */
            }
        } else {
            /* If the caller comes from a cpl_*_save() then we are free to
               assume that no other thread is inside a CPL I/O function
               concerning the same file. We can therefore unset and close all
               reader file pointers open for that filename. A write file pointer
               if present is not unset, since it can be reused. */

            while ((*pfptr = cpl_io_fits_unset_fptr(filename, &rmiomode))
                   != NULL && !fits_close_file(*pfptr, status)) {
#ifdef _OPENMP
#pragma omp atomic
#endif
                cpl_nfitsfiles--;

#ifdef CPL_IO_FITS_DEBUG
                cpl_msg_debug(cpl_func, "Closed file: %s (%p) (I/O-mode: %d != "
                              "%d) (%d)", filename, (const void*)*pfptr,
                              rmiomode, iomode, (int)cpl_nfitsfiles);
#endif
            }

            if (*status) {
#ifdef CPL_IO_FITS_DEBUG
                cpl_msg_debug(cpl_func, "Could not close file: %s (%p) (I/O-"
                              "mode: %d) (%d)", filename, (const void*)*pfptr,
                              rmiomode, (int)cpl_nfitsfiles);
#endif
                return *status;
            }
        }

        if (*pfptr == NULL) {
            /* Determine if an already open file can be reused */
            /* If iomode is READONLY, then the tid must match */
            /* If iomode is READWRITE, its tid will be set to the current one */
            *pfptr = cpl_io_fits_reuse_fptr(filename, iomode,
					    iomode == READWRITE);
        }

        if (*pfptr != NULL) {

#ifdef CPL_IO_FITS_DEBUG
            cpl_msg_debug(cpl_func, "Reusing handle (%p) for: %s (I/O-mode"
                          ": %d%s) (%d)", (const void*)*pfptr, filename,
                          iomode, iomode == rmiomode ? " for reading" : "",
                          (int)cpl_nfitsfiles);
#endif
#ifdef CPL_IO_FITS_REWIND
	    /* A newly opened file points to the 1st HDU so do the same here */
	    if (fits_movabs_hdu(*pfptr, 1, NULL, status)) {
                (void)cpl_error_fits(iomode == READWRITE
                                     ? CPL_ERROR_FILE_NOT_CREATED
                                     : CPL_ERROR_FILE_NOT_FOUND, status,
                                     fits_movabs_hdu, "filename='%s', mode=%d",
                                     filename, iomode);
#ifdef CPL_IO_FITS_DEBUG
                cpl_msg_debug(cpl_func, "Could not move to primary HDU: %s (%p) (I/O-"
                              "mode: %d) (%d)", filename, (const void*)*pfptr,
                              rmiomode, (int)cpl_nfitsfiles);
#endif
	    }
#endif
            return *status;
        }

        if (cpl_fitslist != NULL) {
            const int tid = omp_get_thread_num();
            cpl_fitsfile_t * oldest = NULL;

#ifdef _OPENMP
            /* Comparison critical with cpl_nfitsfiles increment */
#pragma omp critical(cpl_io_fits)
#endif
            {
                /* Need to open a file. Incerement prior to actual open,
                   in order to avoid the case where a number of synchronized
                   threads first verify that the number of files is (just)
                    below the limit and then the all try to open, thus 
                    exceeding the limit */
                cpl_nfitsfiles++;
                if (cpl_nfitsfiles > cpl_io_max_open) {
                    /* First need to close a file - find it first */
                    oldest = cpl_io_fits_unset_tid(tid);
                }
            }
            if (cpl_io_fits_free(oldest, CPL_FALSE, status)) {
#ifdef _OPENMP
#pragma omp atomic
#endif
                cpl_nfitsfiles--; /* The close failed, so no open */
                return *status;
            }
        }

#ifdef CPL_IO_FITS_DEBUG
        cpl_msg_debug(cpl_func, "Opening file: %s (I/O-mode: %d) (%d)",
                      filename, iomode, (int)cpl_nfitsfiles);
#endif
        if (fits_open_diskfile(pfptr, filename, iomode, status)) {
            (void)cpl_error_set_fits(iomode == READWRITE
                                     ? CPL_ERROR_FILE_NOT_CREATED
                                     : CPL_ERROR_FILE_NOT_FOUND, *status,
                                     fits_open_diskfile,
                                     "filename='%s', mode=%d", filename,
                                     iomode);
#ifdef _OPENMP
#pragma omp atomic
#endif
            cpl_nfitsfiles--; /* The open failed */
        } else {
            cpl_io_fits_set(*pfptr, filename, iomode, iomode == READWRITE);
#ifdef CPL_IO_FITS_DEBUG
            cpl_msg_debug(cpl_func, "Set file: %s (%p) (I/O-mode: %d) (%d)",
                          filename, (const void*)*pfptr, iomode,
                          (int)cpl_nfitsfiles);
#endif
        }
    }

    return *status;
}

/**
 * @internal
 * @brief Instead of closing the file, just flush any written data
 * @param fptr    CFITSIO file pointer to file
 * @param status  Pointer to CFITSIO error status
 * @return        The CFITSIO error status
 * @see fits_flush_file()

  From the 3.280 source code of fits_flush_file():
  Flush all the data in the current FITS file to disk. This ensures that if
  the program subsequently dies, the disk FITS file will be closed correctly.

 */
int cpl_io_fits_close_file(fitsfile *fptr, int *status)
{

    if (*status == 0 && fptr != NULL) { /* Duplicate CFITSIO behaviour */
        int          iomode;
        const char * name = cpl_io_fits_find_name(fptr, &iomode);

        if (name == NULL) {
            /* This branch is used when CPL_IO_MODE is inactive */
            if (fits_close_file(fptr, status)) {
                (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, *status,
                                         fits_close_file, ".");
            }
        } else if (iomode != READONLY) {
#ifdef CPL_IO_FITS_DEBUG
            cpl_msg_debug(cpl_func, "Flushing handle (%p) for: %s (%d) (%d)",
                          (const void*)fptr, name, iomode, (int)cpl_nfitsfiles);
#endif
            if (fits_flush_file(fptr, status)) {
                (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, *status,
                                         fits_flush_file, "filename='%s', "
                                         "mode=%d", name, iomode);
            }
        }
    }

    return *status;
}

/**
 * @internal
 * @brief Wrapper around fits_delete_file (used on error), with a close first
 * @param fptr    CFITSIO file pointer to file
 * @param status  Pointer to CFITSIO error status
 * @return        The CFITSIO error status
 * @see fits_delete_file(), cpl_io_fits_close_file
 * @note This wrapper is needed when CPL_IO_MODE is active

 */
int cpl_io_fits_delete_file(fitsfile *fptr, int *status)
{

    if (fptr != NULL) { /* Duplicate CFITSIO behaviour (*status ignored) */
        int          iomode = 0; /* Initialize, in case the find fails */
        const char * name = cpl_io_fits_find_name(fptr, &iomode);

        cpl_fitsfile_t * cpl_fitsfile = NULL;
        const char * filename = name != NULL && *name == '!' ? name+1 : name;
        cx_list_iterator pos;
        struct stat statbuf;
        const cpl_boolean has_stat = filename ? !stat(filename, &statbuf)
            : CPL_FALSE;

        if (name != NULL) {
            /* CPL_IO_MODE is active */

#ifdef _OPENMP
#pragma omp critical(cpl_io_fits)
#endif
            {
                if (cpl_io_fits_find_fptr(&pos, filename, &iomode,
                                          has_stat ? &statbuf : NULL)) {
                    /* Found it */

                    cpl_fitsfile = (cpl_fitsfile_t *)cx_list_extract(cpl_fitslist,
                                                                     pos);
                }
            }
        }

        if (cpl_fitsfile != NULL) {
            int status_ = 0;
            cpl_io_fits_free(cpl_fitsfile, CPL_TRUE, &status_);
            *status = status_;
        } else {
            int status_ = 0;
            if (fits_delete_file(fptr, &status_)) {
                (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, status_,
                                         fits_delete_file, "filename='%s', "
                                         "I/O-mode: %d <=> %d/%d",
                                         name, iomode, READONLY, READWRITE);
            }
            *status = status_;
        }
    }

    return *status;
}

/**
 * @internal
 * @brief Select the 1st matching pointer structure unsetting it from the list
   @param  tid The thread ID in the pointer structure to match, or -1 for all
   @return The pointer structure to deallocate, or NULL on no match
   @note May not be called when fitslist is empty

 */
static cpl_fitsfile_t * cpl_io_fits_unset_tid(int tid)
{

#ifdef CPL_IO_FITS

    cx_list_iterator pos = cx_list_begin(cpl_fitslist);

    while (pos != cx_list_end(cpl_fitslist)) {

        const cpl_fitsfile_t * cpl_fitsfile =
            (const cpl_fitsfile_t *)cx_list_get(cpl_fitslist, pos);

        if (tid < 0 || cpl_fitsfile->tid == tid) break;

        pos = cx_list_next(cpl_fitslist, pos);
    }

    return pos != cx_list_end(cpl_fitslist)
        ? cx_list_extract(cpl_fitslist, pos) : NULL;
#else
    return NULL;
#endif

}

/**
 * @internal
 * @brief Deallocate one pointer structure, closing or deleting the CFITS file
   @param self    The pointer structure to deallocate, or NULL
   @param dodel   Iff true then delete instead of just closing the file
   @param status  The CFITSIO status
   @return Zero on success or else the CFITSIO status
 */
static
int cpl_io_fits_free(cpl_fitsfile_t * self, cpl_boolean dodel, int * status)
{

    if (self != NULL) {
        if (*status == 0) {
            if (dodel) {
                if (fits_delete_file(self->fptr, status)) {
                    (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, *status,
                                             fits_delete_file, "filename='%s', "
                                             "I/O-mode: %d, Thread-ID: %d",
                                             self->name, self->iomode,
                                             self->tid);
                }
            } else if (fits_close_file(self->fptr, status)) {
                (void)cpl_error_set_fits(CPL_ERROR_BAD_FILE_FORMAT, *status,
                                         fits_close_file, "filename='%s', "
                                         "I/O-mode: %d, Thread-ID: %d",
                                         self->name, self->iomode,
                                         self->tid);
            }

            if (*status == 0) {
#ifdef _OPENMP
#pragma omp atomic
#endif
                cpl_nfitsfiles--;

#ifdef CPL_IO_FITS_DEBUG
                cpl_msg_debug(cpl_func, "Closed oldest file of thread %d: %s "
                              "(I/O-mode: %d. %p) (%d)", self->tid, self->name,
                              self->iomode, (const void*)self->fptr, *status);
#endif
            }

        }

        cpl_free(self->name);
        cpl_free(self);
    }

    return *status;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Close one named file, or all files
  @param filename The file to be closed, NULL will close all 
  @param status   The CFITSIO status
  @return Zero on success or else the CFITSIO status
  @note If no file handle exists for the file, nothing is done
*/
/*----------------------------------------------------------------------------*/
int cpl_io_fits_close(const char * filename, int * status)
{
    if (*status == 0) { /* Duplicate CFITSIO behaviour */
        fitsfile * fptr;

        while ((fptr = cpl_io_fits_unset_fptr(filename, NULL))
               != NULL && !fits_close_file(fptr, status)) {
#ifdef _OPENMP
#pragma omp atomic
#endif
            cpl_nfitsfiles--;
#ifdef CPL_IO_FITS_DEBUG
            cpl_msg_debug(cpl_func, "Closed CFITSIO-file: %p (%s) (%d)",
                          (const void*)fptr, filename, (int)cpl_nfitsfiles);
#endif
        }
    }

    return *status;
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Insert a CFITSIO triplet into the CPL I/O structure
  @param  fptr    The CFITSIO pointer to insert
  @param  name    The filename to insert
  @param  iomode  The I/O mode to insert
  @param  writing CPL_TRUE iff the handle is used for writing
  @void
  @note Since this call is only done after a succesful opening of the named file
        name (and fptr) can safely be assumed to be non-NULL.
*/
/*----------------------------------------------------------------------------*/
static void cpl_io_fits_set(fitsfile * fptr, const char * name, int iomode,
                            cpl_boolean writing)
{
#ifdef CPL_IO_FITS
    if (cpl_fitslist != NULL) {

        char * filename = cpl_strdup(*name == '!' ? name+1 : name);

        struct stat statbuf;
        const cpl_boolean has_stat = !stat(filename, &statbuf);

        cpl_fitsfile_t * cpl_fitsfile = cpl_malloc(sizeof(*cpl_fitsfile));

        /* assert(iomode != READONLY || !writing); */

        cpl_fitsfile->fptr   = fptr;
        cpl_fitsfile->name   = filename;
        cpl_fitsfile->iomode = iomode;
        cpl_fitsfile->tid    = omp_get_thread_num();
        cpl_fitsfile->writing = writing;
        cpl_fitsfile->has_stat = has_stat;

        if (has_stat) {
            cpl_fitsfile->st_dev = statbuf.st_dev;
            cpl_fitsfile->st_ino = statbuf.st_ino; 
        }

#ifdef _OPENMP
#pragma omp critical(cpl_io_fits)
#endif
        {
            cx_list_push_back(cpl_fitslist, (cxcptr)cpl_fitsfile);
        }
    }
#endif
}

#ifdef CPL_IO_FITS
/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Search by name for an already opened FITS file
  @param  pkey     Iff found, *pkey is the location of the entry
  @param  filename The filename to look for, NULL will return first available
  @param  piomode  Iff non-NULL, restrict search to *piomode
  @param  filestat Pointer to stat buffer of filename or NULL when unavailable
  @return CPL_TRUE, iff found
  @note Pointer pkey may not be NULL!
*/
/*----------------------------------------------------------------------------*/
static cpl_boolean cpl_io_fits_find_fptr(cx_list_iterator  * pkey,
                                         const char        * filename,
                                         const int         * piomode,
                                         const struct stat * filestat)
{

    const int tid = omp_get_thread_num();
    const cpl_fitsfile_t * cpl_fitsfile = NULL;

    cpl_size i = 0;
    cx_list_iterator pos = cx_list_begin(cpl_fitslist);
    cpl_boolean found;

    while ((found = pos != cx_list_end(cpl_fitslist))) {

        cpl_fitsfile = (const cpl_fitsfile_t *)cx_list_get(cpl_fitslist, pos);

        if (filename == NULL) break;/* Matches any entry */
        if ((piomode == NULL || *piomode == cpl_fitsfile->iomode) &&
            (((piomode == NULL || *piomode != READONLY)
              && cpl_fitsfile->writing) || cpl_fitsfile->tid == tid) &&
            (filestat != NULL && cpl_fitsfile->has_stat
             ? cpl_fitsfile->st_dev == filestat->st_dev &&
               cpl_fitsfile->st_ino == filestat->st_ino
             : !strcmp(cpl_fitsfile->name, filename))) break;

        pos = cx_list_next(cpl_fitslist, pos);
        i++;
    }

    if (found) {
        *pkey = pos;
#ifdef CPL_IO_FITS_DEBUG
        cpl_msg_debug(cpl_func, "File %s found (%d < %d): %p (I/O-mode: "
                      "%d) (tid: %d <=> %d)", filename, (int)i,
                      (int)cpl_nfitsfiles, (const void*)cpl_fitsfile->fptr,
                      cpl_fitsfile->iomode, tid, cpl_fitsfile->tid);
    } else if (piomode != NULL) {
        cpl_msg_debug(cpl_func, "File %s not found (%d) (I/O-mode: %d, "
                      "tid=%d)", filename, (int)cpl_nfitsfiles, *piomode, tid);
    } else {
        cpl_msg_debug(cpl_func, "File %s not found (%d) (tid=%d)", filename,
                      (int)cpl_nfitsfiles, tid);
#endif
    }

    return found;
}
#endif

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Search by name for an already opened FITS file and unset it
  @param  name    The filename to match, @em NULL will unset and return any
  @param  piomode Iff non-@em NULL, match *piomode, otherwise match any mode
  @return When matched, the CFITSIO pointer structure otherwise NULL
  @note The file is matched when the filename:
    1) is NULL, or else
    2) matches (using stat() if need be) and piomode is NULL, or else
    3) matches (using stat() if need be) and piomode is non-NULL and
       matches the mode of the entry and the mode is not READONLY, or else
    4) matches (using stat() if need be) and piomode is non-NULL and
       matches the mode of the entry and dounset is CPL_TRUE, or else
    5) matches (using stat() if need be) and piomode is non-NULL
       and matches the mode of the entry (which is READONLY) and the thread
       id matches

*/
/*----------------------------------------------------------------------------*/
static fitsfile * cpl_io_fits_unset_fptr(const char * name, const int * piomode)
{

    fitsfile * fptr = NULL;

#ifdef CPL_IO_FITS
    if (cpl_fitslist != NULL) {
        cpl_fitsfile_t * cpl_fitsfile = NULL;
        const char * filename = name != NULL && *name == '!' ? name+1 : name;
        cx_list_iterator pos;
        struct stat statbuf;
        const cpl_boolean has_stat = filename ? !stat(filename, &statbuf)
            : CPL_FALSE;



#ifdef _OPENMP
#pragma omp critical(cpl_io_fits)
#endif
        {
            if (cpl_io_fits_find_fptr(&pos, filename, piomode,
                                      has_stat ? &statbuf : NULL)) {
                /* Found it */

                cpl_fitsfile = (cpl_fitsfile_t *)cx_list_extract(cpl_fitslist,
                                                                 pos);
            }
        }

        if (cpl_fitsfile != NULL) {
            fptr = cpl_fitsfile->fptr;
            cpl_free(cpl_fitsfile->name);
            cpl_free(cpl_fitsfile);
        }
    }
#endif

    return fptr;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Search by name for an already opened FITS file for reuse
  @param  name    The filename to match
  @param  iomode  The I/O mode to match
  @param  writing If the file is opened for read/write, its new writing flag
  @return When matched, the CFITSIO pointer structure otherwise NULL
  @see cpl_io_fits_unset_fptr()
  @note Side-effect: If a write handle is matched for write-reuse (writing
        is CPL_TRUE), then its thread id is set to that of the current thread,
        so it can be unset if there are too many open files - and its writer
        flag is set. If a write handle is matched for read-reuse (writing
        is CPL_FALSE), then its thread id already matches - and its writer
        flag is cleared.
*/
/*----------------------------------------------------------------------------*/
static fitsfile * cpl_io_fits_reuse_fptr(const char * name, int iomode,
                                         cpl_boolean writing)
{

    fitsfile * fptr = NULL;

#ifdef CPL_IO_FITS
    if (cpl_fitslist != NULL) {
        cpl_fitsfile_t * cpl_fitsfile = NULL;
        const char * filename = *name == '!' ? name+1 : name;
        cx_list_iterator pos;
        struct stat statbuf;
        const cpl_boolean has_stat = !stat(filename, &statbuf);

#ifdef _OPENMP
#pragma omp critical(cpl_io_fits)
#endif
        {
            if (cpl_io_fits_find_fptr(&pos, filename, &iomode,
                                      has_stat ? &statbuf : NULL)) {
                /* Found it */

                cpl_fitsfile = (cpl_fitsfile_t *)cx_list_get(cpl_fitslist, pos);
            }

            /* Extend critical section, since it is cheap and just to be sure */
            if (cpl_fitsfile != NULL) {
                /* If we are reading, the file pointer is used by no one else */
                /* If we are writing (i.e. called from within a cpl_*save(),
                   we may assume that no one else is currently using the file */
                fptr = cpl_fitsfile->fptr;
                if (iomode != READONLY) {
                    /* assert( cpl_fitsfile->iomode == READWRITE ); */
                    cpl_fitsfile->tid = omp_get_thread_num();
                    cpl_fitsfile->writing = writing;
                }
            }
        }
    }
#endif

    return fptr;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Try to find by CFITSIO pointer an already opened FITS file
  @param  fptr    The CFITSIO pointer to look for
  @param  piomode When found, the I/O mode
  @return When found, the name otherwise NULL
*/
/*----------------------------------------------------------------------------*/
static const char * cpl_io_fits_find_name(const fitsfile * fptr, int * piomode)
{

    const char * name = NULL;

#ifdef CPL_IO_FITS

    if (cpl_fitslist != NULL) {

#ifdef _OPENMP
#pragma omp critical(cpl_io_fits)
#endif
        {
            cx_list_const_iterator pos = cx_list_begin(cpl_fitslist);

            while (pos != cx_list_end(cpl_fitslist)) {

                const cpl_fitsfile_t * cpl_fitsfile = (const cpl_fitsfile_t *)
                    cx_list_get(cpl_fitslist, pos);

                if (fptr == cpl_fitsfile->fptr) {
                    /* Found it */
                    name     = cpl_fitsfile->name;
                    *piomode = cpl_fitsfile->iomode;
                    break;
                }

                pos = cx_list_next(cpl_fitslist, pos);
            }
        }
    }
#endif

    return name;
}
