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

#include "cpl_error_impl.h"

#include "cpl_errorstate.h"
#include "cpl_msg.h"

#include <cxmessages.h>

#include <string.h>


/**
 * @defgroup cpl_errorstate Handling of multiple CPL errors
 *
 * This module provides functions for error detection and recovery
 * and for producing an error traceback.
 *
 * It is built on top of the @c cpl_error module.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_errorstate.h>
 * @endcode
 */
/**@{*/

/*-----------------------------------------------------------------------------
                                   Defines
 -----------------------------------------------------------------------------*/

static cpl_error error_history[CPL_ERROR_HISTORY_SIZE];

/*-----------------------------------------------------------------------------
                             Internal structures
 -----------------------------------------------------------------------------*/

static struct {

    /* This struct may only be accessed when cpl_error_is_set() is true */

    size_t current;  /* The number of the current CPL error in the history */

    /* Numbering of errors start with 1. */

    /* When a new CPL error is created, estate.current is incremented
       holding the number of that error */

    size_t highest;   /* The highest number reached in the history */

    /* cpl_errorstate_set() may not increase estate.current above this value,
       assert( estate.current <= estate.highest ); */

    /* The creation of a new CPL error will set estate.highest to
       estate.current. */

    /* cpl_errorstate_set() may decrease estate.current (to a value
       less than estate.highest) */

    /* NB: Extra: cpl_errorstate_set() may not at all increase estate.current.
       This is to prevent the number from pointing to an error that
       has been changed since the corresponding cpl_errorstate_get() was
       called.
       This means that estate.current can only be increased (incremented,
       that is) when a new CPL error is created. */

    size_t lowest; /* The lowest number for which the error is preserved */

    /* Only for a limited number of errors can the details be preserved.
       These errors are numbered from estate.lowest to estate.highest.

       When estate.lowest exceeds estate.highest then no errors are preserved,
       otherwise details of 1 + estate.highest - estate.lowest errors are
       preserved.

       When the first error is created, estate.lowest is set to 1.
       Is remains at this value, until the error history flows over,
       at which point it starts to increment (cyclically).

    */

    size_t newest; /* The index pointing to the newest CPL error
                        preserved in error_history[] */

    /* estate.newest indexes the CPL error struct of the CPL error with
       the number estate.highest. */

    size_t oldest; /* The index pointing to the oldest CPL error
                        preserved in error_history[] */

    /* estate.oldest == estate.newest means that one CPL error is preserved */

    /* (estate.newest+1)%CPL_ERROR_HISTORY_SIZE == estate.oldest
       means that CPL_ERROR_HISTORY_SIZE CPL errors are preserved, and that
       creating a new CPL error will erase the oldest of the preserved errors */

    /* If estate.current == estate.highest and a new CPL error is created,
       then estate.newest is incremented (cyclicly) and
       the new CPL error is preserved there.
       If this causes estate.oldest == estate.newest, then
       estate.oldest is incremented (cyclicly) as well - and the oldest
       preserved error has been lost. */

    /* If estate.highest - estate.current >= CPL_ERROR_HISTORY_SIZE
       and a new CPL error is created, then all preserved errors
       have been lost. In this case estate.newest is set to estate.oldest
       and the new CPL error is preserved there. */

    /* If diff = estate.highest - estate.current, diff > 0 and
       diff < CPL_ERROR_HISTORY_SIZE and a new CPL error is created, 
       then estate.newest is decremented (cyclically) by diff-1, estate.highest
       is decremented by diff-1, estate.current is incremented (cyclicly),
       and the new error is preserved there.
       estate.oldest is unchanged. */

    /* No binary operator should combine estate.(oldest|newest)
       and estate.(lowest|current|highest). */

} estate;

#ifdef _OPENMP
#pragma omp threadprivate(error_history, estate)
#endif

/*-----------------------------------------------------------------------------
                              Private Function Declarations
 -----------------------------------------------------------------------------*/
static void cpl_errorstate_dump_one_level(void (*)(const char *,
                                                   const char *, ...)
                                          CPL_ATTR_PRINTF(2, 3),
                                          unsigned, unsigned, unsigned);

/*-----------------------------------------------------------------------------
                                   Functions code
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief    Get the CPL errorstate
  @return   The CPL errorstate
  @note The caller should not modify the returned value nor
        transfer it to another function/scope.

  Example of usage:
    @code

        cpl_errorstate prev_state = cpl_errorstate_get();

        // (Call one or more CPL functions here)

        if (cpl_errorstate_is_equal(prev_state)) {
            // No error happened
        } else {
            // One or more errors happened

            // Recover from the error(s)
            cpl_errorstate_set(prev_state);
        }

   @endcode

 */
/*----------------------------------------------------------------------------*/
cpl_errorstate cpl_errorstate_get(void)
{

    /* The return value is defined as the number of errors that have
       happened (since the most recent reset of the error state).
       This definition should be changable without effects to the API.
    */

    /* FIXME: The result is cast to a const pointer of type void, just to
       cause compiler warnings if the caller tries to modify the returned
       value. A better (opaque) typedef would be preferable. */

    return (cpl_errorstate)(cpl_error_is_set() ? estate.current : 0);
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Set the CPL errorstate
  @param    self  The return value of a previous call to cpl_errorstate_get()
  @return   void
  @see cpl_errorstate_get
  @note If a CPL error was created before the call to cpl_errorstate_get()
  that returned self and if more than CPL_ERROR_HISTORY_SIZE CPL errors was
  created after that, then the accessor functions of the CPL error object
  (cpl_error_get_code() etc.) will return wrong values.
  In this case cpl_error_get_code() is still guaranteed not to return
  CPL_ERROR_NONE.
  Moreover, the default value of CPL_ERROR_HISTORY_SIZE is guaranteed to be
  large enough to prevent this situation from arising due to a call of a CPL
  function. 

 */
/*----------------------------------------------------------------------------*/
void cpl_errorstate_set(cpl_errorstate self)
{

    const size_t myself = (const size_t)self;

    if (myself == 0) {
        cpl_error_reset();
    } else if (myself < estate.current) {
        if (!cpl_error_is_readonly()) estate.current = myself;
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Detect a change in the CPL error state
  @param    self  The return value of a previous call to cpl_errorstate_get()
  @return   CPL_TRUE iff the current error state is equal to self.
  @see cpl_errorstate_get

 */
/*----------------------------------------------------------------------------*/
cpl_boolean cpl_errorstate_is_equal(cpl_errorstate self)
{

    const size_t myself = (const size_t)self;

    if (cpl_error_is_set()) {
        return myself == estate.current ? CPL_TRUE : CPL_FALSE;
    } else {
        return myself == 0 ? CPL_TRUE : CPL_FALSE;
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Dump the CPL error state
  @param    self      Dump errors more recent than self
  @param    reverse   Reverse the chronological order of the dump
  @param    dump_one  Function that dumps a single CPL error, or NULL
  @return   void
  @note dump_one may be NULL, in that case cpl_errorstate_dump_one() is called.
        If there are no CPL errors to be dumped, (*dump_one)() is called once
        with all zeros, which allows the dump-function to report that there are
        no errors to dump.
        During calls to (*dump_one)() the CPL error system has been put into
        a special read-only mode that prevents any change of the CPL error
        state. This means that any calls from (*dump_one)() to
        cpl_error_reset(), cpl_error_set(), cpl_error_set_where() and
        cpl_errorstate_set() have no effect. Also calls from (*dump_one)()
        to cpl_errorstate_dump() have no effect.
  @see cpl_errorstate_dump_one

  CPL-based code with insufficient error checking can generate a large number
  of CPL errors. To avoid that a subsequent call to cpl_errorstate_dump() will
  fill up the output device, calls to the dump-function are skipped and only
  counted and the count reported when dump_one is NULL and the CPL error code
  is CPL_ERROR_HISTORY_LOST.

  Example of usage:
    @code

        cpl_errorstate prev_state = cpl_errorstate_get();

        // Call one or more CPL functions

        if (cpl_errorstate_is_equal(prev_state)) {
            // No error happened
        } else {
            // One or more errors happened

            // Dump the error(s) in chronological order
            cpl_errorstate_dump(prev_state, CPL_FALSE,
                                cpl_errorstate_dump_one);

            // Recover from the error(s)
            cpl_errorstate_set(prev_state);
        }

   @endcode

 */
/*----------------------------------------------------------------------------*/
void cpl_errorstate_dump(cpl_errorstate self,
                         cpl_boolean reverse,
                         void (*dump_one)(unsigned, unsigned, unsigned))
{

    if (!cpl_error_is_readonly()) {

        cpl_boolean did_call = CPL_FALSE;
        void (*mydump_one)(unsigned, unsigned, unsigned)
            = dump_one ? dump_one : cpl_errorstate_dump_one;

        /* Put CPL error system into read-only mode,
           to prevent changes by (*dump_one)() */

        cpl_error_set_readonly();

        if (cpl_error_is_set()) {
            const size_t newest = estate.current;
            /* Dump all errors more recent than self */
            const size_t oldest = 1 + (size_t)self;

            if (oldest <= newest) {
                /* There is at least one error to dump */
                const size_t first = reverse ? newest : oldest;
                const size_t last  = reverse ? oldest : newest;
                size_t       ilost = 0;

                /* The iteration through the sequence of errors
                   is done by modifying estate.current */
                if (reverse) {
                    cx_assert(estate.current == first);
                } else {
                    estate.current = first;
                }

                for (; ; reverse ? estate.current-- : estate.current++) {
                    cx_assert( estate.current > 0 );

                    if (dump_one == NULL &&
                        cpl_error_get_code() == CPL_ERROR_HISTORY_LOST) {
                        ilost++;
                    } else {
                        if (ilost > 0) {
                            cpl_msg_error(cpl_func, "Lost %zu CPL error(s)",
                                          ilost);
                            ilost = 0;
                        }
                        (*mydump_one)((unsigned)estate.current, (unsigned)first,
                                      (unsigned)last);
                    }
                    if (estate.current == last) break;
                }
                if (ilost > 0) {
                    cpl_msg_error(cpl_func, "Lost %zu CPL error(s)", ilost);
                    ilost = 0;
                }

                if (reverse) {
                    /* Reestablish the CPL error state */
                    estate.current = newest;
                } else {
                    cx_assert(estate.current == newest);
                }
                did_call = CPL_TRUE;
            }
        }

        if (did_call == CPL_FALSE) {
            /* There are no errors more recent than self.
               Nevertheless give the dumper the opportunity to report that
               the CPL error state is empty */
            (*mydump_one)(0, 0, 0);
        }

        /* Put CPL error system back into normal read/write mode */
        cpl_error_reset_readonly();
    }
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a single CPL error
  @param    self      The number of the current error to be dumped
  @param    first     The number of the first error to be dumped
  @param    last      The number of the last error to be dumped
  @return   void
  @note This function is implemented using only exported CPL functions.
  @see cpl_errorstate_dump

  This function will dump a single CPL error, using the CPL error accessor
  functions. The error is numbered with the value of self.

  The actual output is produced by cpl_msg_error().

  first and last are provided in the API to allow for special messaging in the
  dump of the first and last errors.

  Alternative functions for use with cpl_errorstate_dump() may use all accessor
  functions of the CPL error module and those functions of the CPL message
  module that produce messages. Additionally, the indentation functions of the
  CPL message module may be used, provided that the indentation is set back to
  its original state after the last error has been processed.

 */
/*----------------------------------------------------------------------------*/
void cpl_errorstate_dump_one(unsigned self, unsigned first, unsigned last)
{

    cpl_errorstate_dump_one_level(cpl_msg_error, self, first, last);
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a single CPL error using cpl_msg_warning()
  @param    self      The number of the current error to be dumped
  @param    first     The number of the first error to be dumped
  @param    last      The number of the last error to be dumped
  @return   void
  @see cpl_errorstate_dump_one, cpl_msg_warning()

 */
/*----------------------------------------------------------------------------*/
void cpl_errorstate_dump_one_warning(unsigned self, unsigned first,
                                     unsigned last)
{

    cpl_errorstate_dump_one_level(cpl_msg_warning, self, first, last);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a single CPL error using cpl_msg_info()
  @param    self      The number of the current error to be dumped
  @param    first     The number of the first error to be dumped
  @param    last      The number of the last error to be dumped
  @return   void
  @see cpl_errorstate_dump_one, cpl_msg_info()

 */
/*----------------------------------------------------------------------------*/
void cpl_errorstate_dump_one_info(unsigned self, unsigned first, unsigned last)
{

    cpl_errorstate_dump_one_level(cpl_msg_info, self, first, last);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a single CPL error using cpl_msg_debug()
  @param    self      The number of the current error to be dumped
  @param    first     The number of the first error to be dumped
  @param    last      The number of the last error to be dumped
  @return   void
  @see cpl_errorstate_dump_one, cpl_msg_debug()

 */
/*----------------------------------------------------------------------------*/
void cpl_errorstate_dump_one_debug(unsigned self, unsigned first, unsigned last)
{

    cpl_errorstate_dump_one_level(cpl_msg_debug, self, first, last);
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Append a CPL error to the CPL error state
  @return  A pointer to a CPL error struct to be filled by the CPL error module
  @note This function may only be used by the cpl_error module.

 */
/*----------------------------------------------------------------------------*/
cpl_error * cpl_errorstate_append(void)
{

    if (cpl_error_is_set()) {

        if (estate.lowest > estate.current) {
            /* The details of the current CPL error have been lost */

            /* All preexisting errors are overwritten by making
               estate.newest and estate.oldest equal to one another */
            /* (They could be set to zero, or any other value less than
               CPL_ERROR_HISTORY_SIZE) */
            estate.newest = estate.oldest;

            /* When returning, estate.lowest == estate.current */
            estate.lowest = estate.current+1;
        } else if (estate.highest == estate.current) {

            /* The current error is the newest */

            /* Advance the index of the newest entry in the CPL error history */
            if (++estate.newest == CPL_ERROR_HISTORY_SIZE) estate.newest = 0;

            /* If the history is full, then let the oldest be overwritten */
            if (estate.newest == estate.oldest) {
                estate.lowest++;
                if (++estate.oldest == CPL_ERROR_HISTORY_SIZE) estate.oldest = 0;
            }

        } else {

            const size_t diff = estate.highest - estate.current;

            cx_assert(diff < CPL_ERROR_HISTORY_SIZE);

            /* The current error is preserved, but is not the newest */

            /* An error was created after the current one - this error
               will be overwritten by the new one. */
            /* If more errors were created after that one (i.e. diff > 1),
               then those errors are discarded as well */

            if (estate.newest < diff - 1) {
                estate.newest += CPL_ERROR_HISTORY_SIZE - (diff - 1);
            } else {
                estate.newest -= diff - 1;
            }
        }

        estate.current++;

    } else {
        /* Initialize a new history of errors */
        estate.lowest = estate.current = 1;
        estate.newest  = estate.oldest = 0;
        /* (newest and oldest could also be set to any other value less than
           CPL_ERROR_HISTORY_SIZE) */
    }

    /* The new current error is the newest */
    estate.highest = estate.current;

    return &(error_history[estate.newest]);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Find the CPL error struct used by the current CPL error
  @return   A read-only pointer to the current CPL error struct
  @note This function may only be used by the cpl_error module and only when a
        CPL error is set.
        If the error struct is no longer preserved, the return value will point
        to a valid (read-only) struct containing CPL_ERROR_HISTORY_LOST and an
        empty location.

 */
/*----------------------------------------------------------------------------*/
const cpl_error * cpl_errorstate_find(void)
{

    const cpl_error * self;

    /* assert(cpl_error_is_set()); */

    if (estate.lowest > estate.current) {
        /* The details of the current CPL error have been lost */

        static const cpl_error lost = {CPL_ERROR_HISTORY_LOST, 0, "", "", ""};

        self = &lost;

    } else {

        /* The current CPL error is the newest */
        /* or */
        /* The current CPL error is preserved, but is not the newest */

        const size_t diff = estate.highest - estate.current;

        const size_t current = estate.newest < diff
            ? estate.newest + (CPL_ERROR_HISTORY_SIZE - diff)
            : estate.newest - diff;


        /* assert(diff < CPL_ERROR_HISTORY_SIZE); */

        self = error_history + current;

    }

    return self;

}


/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a single CPL error
  @param    Pointer to one of cpl_msg_info(), cpl_msg_warning(), ...
  @param    self      The number of the current error to be dumped
  @param    first     The number of the first error to be dumped
  @param    last      The number of the last error to be dumped
  @return   void
  @see irplib_errorstate_dump_one

 */
/*----------------------------------------------------------------------------*/
static
void cpl_errorstate_dump_one_level(void (*messenger)(const char *,
                                                        const char *, ...),
                                      unsigned self, unsigned first,
                                      unsigned last)
{


    const cpl_boolean is_reverse = first > last ? CPL_TRUE : CPL_FALSE;
    const unsigned    newest     = is_reverse ? first : last;
    const unsigned    oldest     = is_reverse ? last : first;
    const char      * revmsg     = is_reverse ? " in reverse order" : "";


    cx_assert( messenger != NULL );
    cx_assert( oldest    <= self );
    cx_assert( newest    >= self );

    if (newest == 0) {
        messenger(cpl_func, "No error(s) to dump");
        cx_assert( oldest == 0);
    } else {
        cx_assert( oldest > 0);
        cx_assert( newest >= oldest);
        if (self == first) {
            if (oldest == 1) {
                messenger(cpl_func, "Dumping all %u error(s)%s:", newest,
                          revmsg);
            } else {
                messenger(cpl_func, "Dumping the %u most recent error(s) "
                              "out of a total of %u errors%s:",
                              newest - oldest + 1, newest, revmsg);
            }
            cpl_msg_indent_more();
        }

        messenger(cpl_func, "[%u/%u] '%s' (%u) at %s", self, newest,
                      cpl_error_get_message(), cpl_error_get_code(),
                      cpl_error_get_where());

        if (self == last) cpl_msg_indent_less();
    }
}

