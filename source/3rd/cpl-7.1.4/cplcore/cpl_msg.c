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
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifdef HAVE_TERMIOS_H
#  include <termios.h>
#else
#  ifdef HAVE_TERMIO_H
#    include <termio.h>
#  else
#    error Neither termios.h nor termio.h found!
#  endif
#endif

#ifdef HAVE_STROPTS_H
#  include <stropts.h>
#endif

#if !defined(HAVE_STROPTS_H) || defined(HAVE_TERMIOS_H) || \
    defined(GWINSZ_IN_SYS_IOCTL)
#  ifdef HAVE_SYS_IOCTL_H
#    include <sys/ioctl.h>
#  else
#    error Cannot find header file for ioctl()!
#  endif
#endif


#undef CPL_HAVE_STREAM_DUPLICATION
#undef CPL_HAVE_WINDOW_RESIZING

#ifndef __STRICT_ANSI__
  /* gcc -ansi and gcc -std=... enters here */
#if defined HAVE_FILENO && defined HAVE_FDOPEN && defined HAVE_DUP
#if defined HAVE_DECL_FILENO && defined HAVE_DECL_FDOPEN && defined HAVE_DECL_DUP
#define CPL_HAVE_STREAM_DUPLICATION
#if defined HAVE_SIGACTION && defined HAVE_SIGEMPTYSET
#define CPL_HAVE_WINDOW_RESIZING
#endif
#endif
#endif
#endif

#ifdef _OPENMP
#  include <omp.h>
#endif

#include <cxutils.h>
#include <cxmessages.h>

#include <cpl_error_impl.h>
#include <cpl_msg.h>
#include <cpl_memory.h>


/**
 * @defgroup cpl_msg Messages
 *
 * This module provides functions to display and log messages.
 * The following operations are supported:
 *
 *     - Enable messages output to terminal or to log file.
 *     - Optionally adding informative tags to messages.
 *     - Setting width for message line wrapping.
 *     - Control the message indentation level.
 *     - Filtering messages according to their severity level.
 *
 * To activate and deactivate the messaging system, the functions
 * @c cpl_msg_init() and @c cpl_msg_stop() need to be used. However,
 * since they are called anyway by the functions @c cpl_init() and
 * @c cpl_end(), there is generally no need to call them explicitly,
 * and starting from CPL 2.1 they are deprecated.
 * These functions would typically be called at the beginning and at
 * the end of a program. An attempt to use an uninitialised messaging
 * system would generate a warning message. More functions may also
 * be used to configure the messaging system, and here is an example
 * of a possible initialisation:
 *
 * @code
 *   ...
 *   cpl_msg_set_time_on();
 *   cpl_msg_set_component_on();
 *   cpl_msg_set_domain("Source detection");
 *   cpl_msg_set_domain_on();
 *   cpl_msg_set_level(CPL_MSG_ERROR);
 *   cpl_msg_set_log_level(CPL_MSG_DEBUG);
 *   ...
 * @endcode
 *
 * The functions of these kind, are meant to configure the messaging
 * system, defining its "style", once and for all. For this reason
 * such functions are not supposed to be called from threads.
 * Three different tags may be attached to any message: @em time,
 * @em domain, and @em component. The @em time tag is the time
 * of printing of the message, and can optionally be turned
 * on or off with the functions @c cpl_msg_set_time_on() and
 * @c cpl_msg_set_time_off(). The @em domain tag is an identifier
 * of the main program running (typically, a pipeline recipe),
 * and can be optionally turned on or off with the functions
 * @c cpl_msg_set_domain_on() and @c cpl_msg_set_domain_off().
 * Finally, the @em component tag is used to identify a component
 * of the program running (typically, a function), and can be optionally
 * turned on or off with the functions @c cpl_msg_set_component_on()
 * and @c cpl_msg_set_component_off(). As a default, none of the
 * above tags are attached to messages sent to terminal. However,
 * all tags are always used in messages sent to the log file. A
 * further tag, the @em severity tag, can never be turned off.
 * This tag depends on the function used to print a message, that
 * can be either @c cpl_msg_debug(), @c cpl_msg_info(), @c cpl_msg_warning(),
 * or @c cpl_msg_error(). The @em time and @em severity tags are
 * all prepended to any message, and are not affected by the message
 * indentation controlled by the functions @c cpl_msg_indent(),
 * @c cpl_msg_indent_more(), @c cpl_msg_indent_less(), and
 * @c cpl_msg_set_indentation().
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_msg.h>
 * @endcode
 */

/**@{*/

/*
 *  This is the length for a time string in ISO 8601 format
 */

#define TIME_ISO8601_LENGTH (20)

/*
 *  This is the max length for text lines that are written to the log file.
 *  It is also the max length for text lines sent to the terminal, in case
 *  the window size cannot be determined by the appropriate call to ioctl().
 *  If this number is zero or negative, then lines are not splitted.
 */

#define DEFAULT_WIDTH       (-1)


/*
 *  Strings used for the severity field in the message:
 */

#define ERROR_STRING   "[ ERROR ] "
#define WARNING_STRING "[WARNING] "
#define INFO_STRING    "[ INFO  ] "
#define DEBUG_STRING   "[ DEBUG ] "

inline static
void cpl_msg_out(cpl_msg_severity, const char *, int,
                 const char *, va_list) CPL_ATTR_PRINTF(4,0);

static const char       default_component[]             = "<empty field>";
static const char       default_format[]                = "<empty message>";

static cpl_msg_severity log_min_level                   = CPL_MSG_OFF;
static cpl_msg_severity term_min_level                  = CPL_MSG_INFO;
static int              time_tag                        = 0;
static int              threadid_tag                    = 0;
static int              domain_tag                      = 0;
static int              component_tag                   = 0;
static int              msg_init                        = 0;

static char             domain[CPL_MAX_DOMAIN_NAME]     = "Undefined domain";
static char             logfile_name[CPL_MAX_LOGFILE_NAME]  = ".logfile";
static FILE            *logfile                         = NULL;

static int              page_width                      = DEFAULT_WIDTH;
static const int        log_width                       = DEFAULT_WIDTH;
static int              indent_step                     = 2;
static int              indent_value                    = 0;
static int              overwrite                       = 0;

#ifdef _OPENMP
#pragma omp threadprivate(indent_value, overwrite)
#endif

static FILE            *msg_stdout;
static FILE            *msg_stderr;

#ifdef CPL_HAVE_STREAM_DUPLICATION
static int              out_stream;
#ifdef CPL_HAVE_WINDOW_RESIZING
static struct sigaction act, oact;
#endif
#endif

static cx_print_func    default_printer;
static cx_print_func    default_error;


/*
 * @brief
 *   Ensure system initialisation if it was forgotten.
 *
 * @return Nothing.
 *
 * This private function is used to call cpl_msg_init() if it was not
 * called by the user.
 */

inline static void _cpl_msg_init(const char *component)
{
    if (msg_init == 0) {
        if (cpl_msg_init() == CPL_ERROR_NONE) {
            cpl_msg_warning("CPL messaging",
            "The CPL messaging function %s() was called before the system "
            "had been initialised. Please call the function cpl_init() "
            "before attempting to use any CPL function.", component);
        }
        else {
            fprintf(stderr, "%s\n", cpl_error_get_message());
            fprintf(stderr, "SEVERE ERROR: The CPL messaging system has "
            "not been initialised, and this may cause undefined program "
            "behaviour: please call the function cpl_init() before "
            "attempting to use any CPL function.");
        }
        msg_init = 1;
    }
}

/*
 * @brief
 *   Get current date and time in ISO8601 format.
 * @param
 *   String of size at least TIME_ISO8601_LENGTH
 *
 * @return void
 *
 * This private function just returns the current time in ISO8601 format.
 */

static void _cpl_timestamp_iso8601(char *timestamp)
{

    char _timestamp[TIME_ISO8601_LENGTH];
    const time_t seconds = time(NULL);

    strncpy(_timestamp, "0000-00-00T00:00:00", TIME_ISO8601_LENGTH);

    if (seconds != ((time_t)-1)) {

        struct tm time_of_day;

        if (localtime_r(&seconds, &time_of_day) != NULL) {

            int _errno = errno;
            strftime(_timestamp, TIME_ISO8601_LENGTH, "%Y-%m-%dT%H:%M:%S",
                     &time_of_day);

            /*
             * POSIX does not specify errno settings for strftime. If there
             * is any, discard it!
             */

            errno = _errno;

        }

    }

    strncpy(timestamp, _timestamp, TIME_ISO8601_LENGTH);
    return;

}

#ifdef CPL_HAVE_STREAM_DUPLICATION

/*
 * @brief
 *  Signal handler for signal @c SIGWINCH
 *
 * @param i  Dummy argument (not used!)
 *
 * @return Nothing.
 *
 * This private function accomodates the output line width of the messaging
 * subsystem to the new window size on arrival of the signal @c SIGWINCH.
 */

static void _cpl_change_width(int i)
{

    struct winsize win;

    CPL_UNUSED(i);

    if (ioctl(out_stream, TIOCGWINSZ, &win) < 0 || win.ws_col < 1)
        page_width = DEFAULT_WIDTH;
    else
        page_width = win.ws_col;

}
#endif


/*
 * @brief
 *  Handler for printing to standard output.
 *
 * @param String to print.
 *
 * @return Nothing.
 *
 * This private function is used by cx_print() to write any message
 * to standard output.
 */

static void _cpl_print_out(const cxchar *message)
{

    fputs(message, msg_stdout);
    fflush(msg_stdout);

}


/*
 * @brief
 *  Handler for printing to standard error.
 *
 * @param String to print.
 *
 * @return Nothing.
 *
 * This private function is used by cx_printerr() to write any message
 * to standard output.
 */

static void _cpl_print_err(const cxchar *message)
{

    fputs(message, msg_stderr);
    fflush(msg_stderr);

}


/*
 * @brief
 *   Split a string according to the max allowed page width.
 *
 * @param split   Processed output string at least of size CPL_MAX_MSG_LENGTH
 * @param s       Input string to be processed.
 * @param blanks  Number of blanks to be inserted at every split point.
 * @param width   Max number of characters between split points.
 *
 * @return Pointer to the modified character string, or if the width is less
 *         than one, pointer to the unmodified input string.
 *
 * This private function is used for splitting a string avoiding to exceed
 * a maximum width (as for instance the width of the terminal where the
 * string is going to be printed). The splitting is performed without
 * breaking words, i.e. by replacing with a newline character ('\\n')
 * the last blank character before the maximum allowed width. Newline
 * characters already present in the input string are preserved.
 * Single words that exceed the max allowed width would not be split,
 * just in this case long lines are tolerated. A number of blanks to
 * be inserted at every split point must be specified, setting the
 * left indentation level for the printed string. This number must
 * not exceed the maximum allowed width.
 */

static const char *strsplit(char * split, const char *s, int blanks, int width)
{

    int i, j, k;
    int cuti  = 0;
    int cutj  = 0;
    int limit = width;


    if (width < 1)
        return s;

    if (blanks >= width)
        blanks = width - 1;        /* Give up indentation */

    for (i = 0, j = 0;
         i < CPL_MAX_MSG_LENGTH && j < CPL_MAX_MSG_LENGTH; i++, j++) {

        split[j] = s[i];

        if (s[i] == ' ' || s[i] == '\0' || s[i] == '\n') {

            if (i >= limit) {

               /*
                *  Go back to the previous cuttable position, if possible
                */

                if (limit - cuti < width - blanks) {
                    j = cutj;
                    i = cuti;
                }
                else {
                    if (s[i] == '\0')
                        break;
                }

               /*
                *  Split here, and insert blanks
                */

                split[j] = '\n';

                for (k = 0, j++;
                     k < blanks && j < CPL_MAX_MSG_LENGTH; k++, j++)
                    split[j] = ' ';
                j--;

                limit = width - blanks + i;
            }
            else {
                if (s[i] == '\0')
                      break;

                if (s[i] == '\n') {

                   /*
                    *  Split point already present in input string: just add
                    *  the specified number of blanks
                    */

                    if (s[i+1] == '\0') {
                        split[j] = '\0';
                        break;
                    }

                    for (k = 0, j++;
                         k < blanks && j < CPL_MAX_MSG_LENGTH; k++, j++)
                        split[j] = ' ';
                    j--;

                    limit = width - blanks + i;
                }

               /*
                *  Keep track of the last cuttable position
                */

                cutj = j;
                cuti = i;

            }
        }
    }


   /*
    *  Safety belt!
    */

    split[CPL_MAX_MSG_LENGTH - 1] = '\0';

    return split;

}


/*
 * @brief
 *   Format and output message string.
 *
 * @param severity      Severity level of the incoming message.
 * @param component     Name of the component/function generating the message.
 * @param caller        1 = cpl_msg_info_overwritable, 0 = all the others.
 * @param format        Format string in the usual C convention.
 * @param al            Variable argument list associated to the @em format.
 *
 * @return Nothing.
 *
 * This private function is used to actually display/add the message
 * to terminal and/or log file. Messages with severity level equal to
 * "error" or greater would be sent to stderr, the other messages
 * would go to stdout.
 *
 * If the severity level is lower than the levels set by
 * @b cpl_msg_set_level() and @b cpl_msg_set_log_level(), then
 * the message is not displayed.
 *
 * @see cpl_msg_set_level(), cpl_msg_set_log_level()
 */

inline static void cpl_msg_out(cpl_msg_severity severity,
                               const char *component, int caller,
                               const char *format, va_list al)
{
    struct tm time_of_day;
    time_t seconds;
    char   msg_text[CPL_MAX_MSG_LENGTH] = "";
    char   msg_log[CPL_MAX_MSG_LENGTH] = "";
    char   msg_term[CPL_MAX_MSG_LENGTH] = "";
    char   split[CPL_MAX_MSG_LENGTH];

#ifdef _OPENMP
    char  *tid;
#endif
    int    start_log_line, start_term_line;
    int    copy_only;
    int    i;


    if (severity < term_min_level && severity < log_min_level)
        return;

    if (severity == CPL_MSG_OFF)
        return;

    seconds = time(NULL);

    cx_vsnprintf(msg_text, CPL_MAX_MSG_LENGTH, format, al);


   /*
    *  Date and time. Note that time tag and severity field are not
    *  affected by indentation. Date and time are always present in
    *  the log file, optional in the terminal output.
    */

    if (localtime_r(&seconds, &time_of_day) == NULL) {
        msg_log[0]  = '\0';
        msg_term[0] = '\0';
    }
    else {

        /* three 2-digit integers + 2 colons + 1 space + terminating 0 */
        char msg_timestamp[10];

        strftime(msg_timestamp, 10, "%H:%M:%S ", &time_of_day);

        strncpy(msg_log, msg_timestamp, 10);

        if (time_tag) {
            strncpy(msg_term, msg_timestamp, 10);
        }
        else {
            msg_term[0] = '\0';
        }

    }


   /*
    *  Severity label
    */

    if (severity == CPL_MSG_ERROR) {
        strncat(msg_log, ERROR_STRING,
                CPL_MAX_MSG_LENGTH - strlen(msg_log) - 1);
        strncat(msg_term, ERROR_STRING,
               CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1);
    }
    else if (severity == CPL_MSG_WARNING) {
        strncat(msg_log, WARNING_STRING,
               CPL_MAX_MSG_LENGTH - strlen(msg_log) - 1);
        strncat(msg_term, WARNING_STRING,
               CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1);
    }
    else if (severity == CPL_MSG_INFO) {
        strncat(msg_log, INFO_STRING,
               CPL_MAX_MSG_LENGTH - strlen(msg_log) - 1);
        strncat(msg_term, INFO_STRING,
               CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1);
    }
    else if (severity == CPL_MSG_DEBUG) {
        strncat(msg_log, DEBUG_STRING,
               CPL_MAX_MSG_LENGTH - strlen(msg_log) - 1);
        strncat(msg_term, DEBUG_STRING,
               CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1);
    }


   /*
    *  Domain, component name, and message appended:
    */

    if (domain_tag) {
        strncat(msg_term, domain, CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1);
        strncat(msg_term, ": ", CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1);
    }

    if (component_tag || term_min_level == CPL_MSG_DEBUG) {
        strncat(msg_term, component, CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1);
        strncat(msg_term, ": ", CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1);
    }

    strncat(msg_log, component, CPL_MAX_MSG_LENGTH - strlen(msg_log) - 1);
    strncat(msg_log, ": ", CPL_MAX_MSG_LENGTH - strlen(msg_log) - 1);


#ifdef _OPENMP

   /*
    * Thread ID
    */

   tid = cpl_sprintf("[tid=%03d] ", omp_get_thread_num());
   strncat(msg_log, tid, CPL_MAX_MSG_LENGTH - strlen(msg_log) - 1);
   if (threadid_tag)
       strncat(msg_term, tid, CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1);
   cpl_free(tid);

#endif

   /*
    *  Message indentation
    */

    for (i = 0; i < indent_value; i++) {
        strncat(msg_log, " ", CPL_MAX_MSG_LENGTH - strlen(msg_log) - 1);
        strncat(msg_term, " ", CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1);
    }


    start_log_line = strlen(msg_log);
    start_term_line = strlen(msg_term);


   /*
    *  Finally add the message text. If message is too long
    *  it is truncated.
    */

    copy_only = CPL_MAX_MSG_LENGTH - strlen(msg_log) - 1;
    strncat(msg_log, msg_text, copy_only);
    copy_only = CPL_MAX_MSG_LENGTH - strlen(msg_term) - 1;
    strncat(msg_term, msg_text, copy_only);

    if (severity >= log_min_level)
        fprintf(logfile, "%s\n",
                strsplit(split, msg_log, start_log_line, log_width));

    if (severity >= term_min_level) {
        if (severity > CPL_MSG_WARNING) {
            if (overwrite) {
                cx_printerr("\n%s\n", strsplit(split, msg_term,
                                               start_term_line, page_width));
                overwrite = 0;
            }
            else
                cx_printerr("%s\n", strsplit(split, msg_term,
                                             start_term_line, page_width));
        }
        else
            if (caller) {
                char *c = strrchr(msg_term, '\n');
                if (c >= msg_term) *c = '\0';
                cx_print("\r%s", msg_term);
            }
            else
                if (overwrite) {
                    cx_print("\n%s\n", strsplit(split, msg_term,
                                                start_term_line, page_width));
                    overwrite = 0;
                }
                else
                    cx_print("%s\n", strsplit(split, msg_term,
                                              start_term_line, page_width));
    }
}


/**
 * @brief
 *   Initialise the messaging system
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_ALREADY_OPEN</td>
 *       <td class="ecr">
 *         The messaging system was already initialised.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DUPLICATING_STREAM</td>
 *       <td class="ecr">
 *         <tt>stdout</tt> and <tt>stderr</tt> streams cannot be duplicated.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ASSIGNING_STREAM</td>
 *       <td class="ecr">
 *         A stream cannot be associated with a file descriptor.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function needs to be called to activate the messaging system,
 * typically at the beginning of a program. An attempt to use any
 * messaging function before turning the system on would generate
 * a warning message. The messaging system needs to be deactivated
 * by calling the function @c cpl_msg_stop(). However, since these
 * functions are called anyway by the functions @c cpl_init() and
 * @c cpl_end(), there is generally no need to call them explicitly,
 * and starting from CPL 2.1 they are deprecated.
 *
 * When @c cpl_msg_init() is called, the @em stdout and
 * @em stderr streams are duplicated for greater flexibility of
 * the system. The terminal width is determined (if possible),
 * and the resized window signal handler is deployed to monitor
 * possible changes of the terminal window width. If the width of
 * the output device cannot be determined, lines of text are not
 * splitted when written to output. If line splitting is not wanted,
 * the function @c cpl_msg_set_width() should be called specifying
 * a non positive width.
 */

cpl_error_code cpl_msg_init(void)
{

#ifdef CPL_HAVE_STREAM_DUPLICATION
    struct winsize win;
    static int err_stream;
#endif


    if (msg_init > 0)
        return cpl_error_set_(CPL_ERROR_FILE_ALREADY_OPEN);


#ifdef CPL_HAVE_STREAM_DUPLICATION
    /*
     *  First duplicate stdout and stderr streams
     */

    if ((out_stream = dup(fileno(stdout))) < 0)
        return cpl_error_set_(CPL_ERROR_DUPLICATING_STREAM);

    if (!(err_stream = dup(fileno(stderr))))
        return cpl_error_set_(CPL_ERROR_DUPLICATING_STREAM);

    if (!(msg_stdout = fdopen(out_stream, "a")))
        return cpl_error_set_(CPL_ERROR_ASSIGNING_STREAM);

    if (!(msg_stderr = fdopen(err_stream, "a")))
        return cpl_error_set_(CPL_ERROR_ASSIGNING_STREAM);
#else

    msg_stdout = stdout;
    msg_stderr = stderr;

#endif

    default_printer = cx_print_set_handler(_cpl_print_out);
    default_error = cx_printerr_set_handler(_cpl_print_err);

    msg_init = 1;

#ifdef CPL_HAVE_STREAM_DUPLICATION
#ifdef CPL_HAVE_WINDOW_RESIZING
   /*
    *  Get the terminal window size, and if successful deploy the handler
    *  for any image resizing at runtime.
    */

    if (ioctl(out_stream, TIOCGWINSZ, &win) < 0 || win.ws_col < 1)
        return CPL_ERROR_NONE;

    page_width = win.ws_col;

    act.sa_handler = _cpl_change_width;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;            /* Probably more appropriate flags         *
                                  * initialisation should be inserted here. */

    act.sa_flags &= ~SA_SIGINFO; /* Eliminates SA_SIGINFO from any setting  *
                                  * above.                                  */

    sigaction(SIGWINCH, &act, &oact);
#endif
#endif

    /* Setup time zone information for localtime_r() calls in cpl_msg_out() */
    tzset();

    return CPL_ERROR_NONE;
}


/**
 * @brief
 *   Turn the messaging system off.
 *
 * @return Nothing
 *
 * This function needs to be called to turn the messaging system off,
 * typically at the end of a program. To turn the messaging system
 * on the function @c cpl_msg_init() needs to be called. However, since
 * these functions are called anyway by the functions @c cpl_init()
 * and @c cpl_end(), there is generally no need to call them explicitly,
 * and starting from CPL 2.1 they are deprecated.
 *
 * When @c cpl_msg_stop() is called, the default resized window signal
 * handler is restored, and the duplicated output streams are closed.
 * If a log file is still open, it is closed, and the log verbosity
 * level is set to CPL_MSG_OFF. If the messaging system is not on,
 * nothing is done, and no error is set.
 */

void cpl_msg_stop(void)
{
    if (msg_init == 0)
        return;

#ifdef CPL_HAVE_STREAM_DUPLICATION
#ifdef CPL_HAVE_WINDOW_RESIZING
    if (act.sa_handler == _cpl_change_width)
        sigaction(SIGWINCH, &oact, NULL);
#endif
#endif

    cx_print_set_handler(default_printer);
    cx_printerr_set_handler(default_error);

    if (msg_stdout != stdout)
        fclose(msg_stdout);

    if (msg_stderr != stderr)
        fclose(msg_stderr);

    cpl_msg_stop_log();

    msg_init = 0;
}


/**
 * @brief
 *   Open and initialise a log file.
 *
 * @param verbosity  Verbosity level.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_ALREADY_OPEN</td>
 *       <td class="ecr">
 *         A log file was already started.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_NOT_CREATED</td>
 *       <td class="ecr">
 *         Log file cannot be created.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * If the specified @em verbosity level is different from @c CPL_MSG_OFF,
 * a log file is created and initialised with a header containing the
 * start logging time, the @em domain identifier set by the function
 * @c cpl_msg_set_domain(), and the chosen @em verbosity level. The
 * @em verbosity specifies the lowest severity level that a message
 * should have to be written to the log file. The name of the created
 * log file may be previously set with the function @c cpl_msg_set_log_name(),
 * otherwise it is left to a default ".logfile". The log file name can
 * be obtained by calling the function @c cpl_msg_get_log_name().
 * Typically this function is called at the beginning of a program.
 * Calling it while a log file is already open has no effect, but it
 * will return an error code.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

cpl_error_code cpl_msg_set_log_level(cpl_msg_severity verbosity)
{

    _cpl_msg_init(cpl_func);

    if (logfile) {

        /*
         *  If a log file was already open, nothing is done, but a status
         *  is returned.
         */

        return cpl_error_set_(CPL_ERROR_FILE_ALREADY_OPEN);

    }

    if (verbosity != CPL_MSG_OFF) {

        char timeLabel[TIME_ISO8601_LENGTH];

        if ((logfile = fopen(logfile_name, "w")) == NULL)
            return cpl_error_set_message_(CPL_ERROR_FILE_NOT_CREATED, "%s",
                                          logfile_name);

        (void)setvbuf(logfile, (char *) NULL, _IOLBF, 0);

        log_min_level = verbosity;

        /*
         *  Write log file header
         */

        _cpl_timestamp_iso8601(timeLabel);

        fprintf(logfile, "\n");
        fprintf(logfile, "Start time     : %s\n", timeLabel);
        fprintf(logfile, "Program name   : %s\n", domain);
        fprintf(logfile, "Severity level : ");

        switch(verbosity) {
            case CPL_MSG_DEBUG   : fprintf(logfile, DEBUG_STRING); break;
            case CPL_MSG_INFO    : fprintf(logfile, INFO_STRING); break;
            case CPL_MSG_WARNING : fprintf(logfile, WARNING_STRING); break;
            case CPL_MSG_ERROR   : fprintf(logfile, ERROR_STRING); break;
            default              : break;
        }

        fprintf(logfile, "\n\n");
    }

    return CPL_ERROR_NONE;
}


/**
 * @brief
 *   Close the current log file.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * The log file is closed. The name of the created log file is always the same,
 * and can be obtained by calling the function @c cpl_msg_get_log_name().
 * An attempt to close a non existing log file would not generate an error
 * condition. This routine may be called in case the logging should be
 * terminated before the end of a program. Otherwise, the function
 * @c cpl_msg_stop() would automatically close the log file when called
 * at the end of the program.
 */

cpl_error_code cpl_msg_stop_log(void)
{

    _cpl_msg_init(cpl_func);

    if (log_min_level != CPL_MSG_OFF) {

        log_min_level = CPL_MSG_OFF;
        fclose(logfile);
        logfile = NULL;

    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Get the log file name.
 *
 * @return Logfile name
 *
 * The name of the log file is returned.
 */

const char *cpl_msg_get_log_name(void)
{
    _cpl_msg_init(cpl_func);

    return logfile_name;
}


/**
 * @brief
 *   Set the log file name.
 *
 * @param name  Name of log file.
 *
 * @return @c CPL_ERROR_NONE on success.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The specified <i>name</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_FILE_ALREADY_OPEN</td>
 *       <td class="ecr">
 *         A log file was already started.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The specified <i>name</i> is longer than
 *         <tt>CPL_MAX_LOGFILE_NAME</tt> characters (including the
 *         terminating '\\0').
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function is used to set the log file name, and can only be
 * called before the log is opened by @c cpl_msg_set_log_level().
 * If this function is not called, or the specified @em name is
 * longer than <tt>CPL_MAX_LOGFILE_NAME</tt> characters, the log
 * file name is left to its default, ".logfile".
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

cpl_error_code cpl_msg_set_log_name(const char *name)
{

    _cpl_msg_init(cpl_func);

    if (name == NULL)
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    if (logfile)
        return cpl_error_set_message_(CPL_ERROR_FILE_ALREADY_OPEN, "%s: %p",
                                      name, (const void*)logfile);

    if (strlen(name) > CPL_MAX_LOGFILE_NAME - 1)
        return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "%s: %u + 1 > "
                                      CPL_STRINGIFY(CPL_MAX_LOGFILE_NAME) " = "
                                      CPL_XSTRINGIFY(CPL_MAX_LOGFILE_NAME),
                                      name, (unsigned)strlen(name));

    strcpy(logfile_name, name);

    return CPL_ERROR_NONE;
}


/**
 * @brief
 *   Set verbosity level of output to terminal.
 *
 * @param verbosity  Verbosity level.
 *
 * @return Nothing.
 *
 * The @em verbosity specifies the lowest severity level that a message
 * should have for being displayed to terminal. If this function is not
 * called, the verbosity level defaults to @c CPL_MSG_INFO.
 *
 * @note
 *   This function is not supposed to be called in threads.
 */

void cpl_msg_set_level(cpl_msg_severity verbosity)
{

    _cpl_msg_init(cpl_func);

    term_min_level = verbosity;

}


/*----------------------------------------------------------------------------*/
/**
  @brief    Set verbosity level of terminal output using an environment variable
  @return   void
  @see cpl_msg_set_level
  @note This function can be used for run-time control of the verbosity level of
        unit test modules.

  The CPL verbosity level of output to terminal is set according to the
  environment variable CPL_MSG_LEVEL:
  debug:   CPL_MSG_DEBUG
  info:    CPL_MSG_INFO
  warning: CPL_MSG_WARNING
  error:   CPL_MSG_ERROR
  off:     CPL_MSG_OFF

  Any other value (including NULL) will cause the function to do nothing.

 */
/*----------------------------------------------------------------------------*/
void cpl_msg_set_level_from_env(void)
{

    const char * level = getenv("CPL_MSG_LEVEL");

    if (level == NULL) return;

    if (!strcmp(level, "debug"))
        cpl_msg_set_level(CPL_MSG_DEBUG);
    else if (!strcmp(level, "info"))
        cpl_msg_set_level(CPL_MSG_INFO);
    else if (!strcmp(level, "warning"))
        cpl_msg_set_level(CPL_MSG_WARNING);
    else if (!strcmp(level, "error"))
        cpl_msg_set_level(CPL_MSG_ERROR);
    else if (!strcmp(level, "off"))
        cpl_msg_set_level(CPL_MSG_OFF);

    return;
}

/**
 * @brief
 *   Get current log verbosity level.
 *
 * @return Current verbosity level.
 *
 * Get current verbosity level set for the output to the log file.
 */

cpl_msg_severity cpl_msg_get_log_level(void)
{
    _cpl_msg_init(cpl_func);

    return log_min_level;
}


/**
 * @brief
 *   Get current terminal verbosity level.
 *
 * @return Current verbosity level.
 *
 * Get current verbosity level set for the output to terminal.
 */

cpl_msg_severity cpl_msg_get_level(void)
{
    _cpl_msg_init(cpl_func);

    return term_min_level;
}


/**
 * @brief
 *   Attach a @em time tag to output messages.
 *
 * @return Nothing.
 *
 * As a default, @em time tags are attached just to messages written
 * to the log file. This function must be called to display the @em time
 * tag also in messages written to terminal. To turn the @em time tag
 * off the function @c cpl_msg_set_time_off() should be called.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

void cpl_msg_set_time_on(void)
{
    _cpl_msg_init(cpl_func);

    time_tag = 1;
}


/**
 * @brief
 *   Disable the @em time tag in output messages.
 *
 * @return Nothing.
 *
 * The @em time tag is turned off in messages written to terminal.
 * The @em time tag cannot be turned off in messages written to the
 * log file.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

void cpl_msg_set_time_off(void)
{
    _cpl_msg_init(cpl_func);

    time_tag = 0;
}

/**
 * @brief
 *   Attach a @em thread id tag to output messages.
 *
 * @return Nothing.
 *
 * As a default, @em thread ids tags are attached just to messages written
 * to the log file. This function must be called to display the @em thread id
 * tag also in messages written to terminal. To turn the @em thread id tag
 * off the function @c cpl_msg_set_threadid_off() should be called.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

void cpl_msg_set_threadid_on(void)
{
    _cpl_msg_init(cpl_func);

    threadid_tag = 1;
}


/**
 * @brief
 *   Disable the @em thread id tag to output messages
 *
 * @return Nothing.
 *
 * The @em thread id tag is turned off in messages written to terminal.
 * The @em thread id tag cannot be turned off in messages written to the
 * log file.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

void cpl_msg_set_threadid_off(void)
{
    _cpl_msg_init(cpl_func);

    threadid_tag = 0;
}


/**
 * @brief
 *   Attach the @em domain tag to output messages.
 *
 * @return Nothing.
 *
 * As a default, the @em domain tag is just written to the header of
 * the log file. This function must be called to attach the @em domain
 * tag to all messages written to terminal. If the @em domain tag is
 * on and no domain tag was specified, the string "Undefined domain"
 * (or something analogous) would be attached to all messages. To turn
 * the @em domain tag off the function @c cpl_msg_set_domain_off() must
 * be called.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

void cpl_msg_set_domain_on(void)
{
    _cpl_msg_init(cpl_func);

    domain_tag = 1;
}


/**
 * @brief
 *   Disable the @em domain tag in output messages.
 *
 * @return Nothing.
 *
 * The @em domain tag is turned off in messages written to terminal.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

void cpl_msg_set_domain_off(void)
{
    _cpl_msg_init(cpl_func);

    domain_tag = 0;
}


/**
 * @brief
 *   Attach the @em component tag to output messages.
 *
 * @return Nothing.
 *
 * As a default, the @em component tag is attached just to messages written
 * to the log file. This function must be called to display the @em component
 * tag also in messages written to terminal. To turn the @em component tag
 * off the function @c cpl_msg_set_component_off() should be called. However,
 * the @em component tag is always shown when the verbosity level is set
 * to @c CPL_MSG_DEBUG.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

void cpl_msg_set_component_on(void)
{
    _cpl_msg_init(cpl_func);

    component_tag = 1;
}


/**
 * @brief
 *   Disable the @em component tag in output messages.
 *
 * @return Nothing.
 *
 * The @em component tag is turned off in messages written to terminal,
 * unless the verbosity level is set to @c CPL_MSG_DEBUG. The @em component
 * tag cannot be turned off in messages written to the log file.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

void cpl_msg_set_component_off(void)
{
    _cpl_msg_init(cpl_func);

    component_tag = 0;
}


/**
 * @brief
 *   Set the @em domain name.
 *
 * @param name Any task identifier, typically a pipeline recipe name.
 *
 * @return Nothing.
 *
 * This routine should be called at a pipeline recipe start, and
 * before a possible call to the function cpl_msg_set_log_level() or the
 * proper task identifier would not appear in the log file header.
 * The @em domain tag is attached to messages sent to terminal only
 * if the function @c cpl_msg_set_domain_on() is called. If the
 * @em domain tag is on and no domain tag was specified, the string
 * "Undefined domain" (or something analogous) would be attached
 * to all messages. To turn the @em domain tag off the function
 * @c cpl_msg_set_domain_off() should be called. If @em name is a
 * @c NULL pointer, nothing is done, and no error is set.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

void cpl_msg_set_domain(const char *name)
{
    _cpl_msg_init(cpl_func);

    if (name == NULL)
        return;

    if (strlen(name) >= CPL_MAX_DOMAIN_NAME) {
        strncpy(domain, name, CPL_MAX_DOMAIN_NAME);
        domain[CPL_MAX_DOMAIN_NAME-1] = '\0';
    }
    else {
        strcpy(domain, name);
    }
}


/**
 * @brief
 *   Get the @em domain name.
 *
 * @return Pointer to "domain" string.
 *
 * This routine returns always the same pointer to the statically
 * allocated buffer containing the "domain" string.
 */

const char *cpl_msg_get_domain(void)
{
    _cpl_msg_init(cpl_func);

    return domain;
}


/**
 * @brief
 *   Set the maximum width of the displayed text.
 *
 * @param width Max width of the displayed text.
 *
 * @return Nothing.
 *
 * If a message is longer than @em width characters, it would be broken
 * into shorter lines before being displayed to terminal. However, words
 * longer than @em width would not be broken, and in this case longer
 * lines would be printed. This function is automatically called by the
 * messaging system every time the terminal window is resized, and the
 * width is set equal to the new width of the terminal window. If @em width
 * is zero or negative, long message lines would not be broken. Lines are
 * never broken in log files.
 */

void cpl_msg_set_width(int width)
{
    _cpl_msg_init(cpl_func);

    if (width < 0)
        width = 0;

    page_width = width;
}


/**
 * @brief
 *   Set the indentation step.
 *
 * @param step Indentation step.
 *
 * @return Nothing.
 *
 * To maintain a consistent message display style, this routine
 * should be called at most once, and just at program start.
 * A message line might be moved leftward or rightward by a
 * number of characters that is a multiple of the specified
 * indentation step. Setting the indentation step to zero or
 * to a negative number would eliminate messages indentation.
 * If this function is not called, the indentation step defaults to 2.
 *
 * @note
 *   This function is meant to configure once and for all the behaviour
 *   and the "style" of the messaging system, and it is not supposed to
 *   be called in threads.
 */

void cpl_msg_set_indentation(int step)
{
    _cpl_msg_init(cpl_func);

    if (step < 0)
        step = 0;

    indent_step = step;
}


/**
 * @brief
 *   Set the indentation level.
 *
 * @return Nothing.
 *
 * @param level Indentation level.
 *
 * Any message printed after a call to this function would be indented
 * by a number of characters equal to the @em level multiplied by the
 * indentation step specified with the function @c cpl_msg_set_indentation().
 * Specifying a negative indentation level would set the indentation
 * level to zero.
 */

void cpl_msg_indent(int level)
{
    _cpl_msg_init(cpl_func);

    if (level < 0)
        level = 0;

    indent_value = level * indent_step;
}


/**
 * @brief
 *   Increase the message indentation by one indentation step.
 *
 * @return Nothing.
 *
 * Calling this function is equivalent to increase the indentation
 * level by 1. See function @c cpl_msg_indent().
 */

void cpl_msg_indent_more(void)
{
    _cpl_msg_init(cpl_func);

    indent_value += indent_step;
}


/**
 * @brief
 *   Decrease the message indentation by one indentation step.
 *
 * @return Nothing.
 *
 * Calling this function is equivalent to decrease the indentation level
 * by 1. If the indentation level is already 0, it is not decreased.
 */

void cpl_msg_indent_less(void)
{
    _cpl_msg_init(cpl_func);

    if (indent_value > 0)
        indent_value -= indent_step;
}


/**
 * @brief
 *   Display an error message.
 *
 * @return Nothing.
 *
 * @param component Name of the component generating the message.
 * @param format    Format string.
 * @param ...       Variable argument list associated to the format string.
 *
 * The @em format string should follow the usual @c printf() convention.
 * Newline characters shouldn't generally be used, as the message
 * would be split automatically according to the width specified with
 * @b cpl_msg_set_width(). Inserting a newline character would
 * enforce breaking a line of text even before the current row is
 * filled. Newline characters at the end of the @em format string
 * are not required. If @em component is a @c NULL pointer, it would
 * be set to the string "<empty field>". If @em format is a @c NULL
 * pointer, the message "<empty message>" would be printed.
 */

void cpl_msg_error(const char *component, const char *format, ...)
{

    const char *c = component != NULL ? component : default_component;

    _cpl_msg_init(cpl_func);

    if (format == NULL) {
        cpl_msg_error(c, default_format);
    } else {
        va_list     al;
        va_start(al, format);
        cpl_msg_out(CPL_MSG_ERROR, c, 0, format, al);
        va_end(al);
    }
}


/**
 * @brief
 *   Display a warning message.
 *
 * @return Nothing.
 *
 * @param component  Name of the function generating the message.
 * @param format     Format string.
 * @param ...        Variable argument list associated to the format string.
 *
 * See the description of the function @c cpl_msg_error().
 */

void cpl_msg_warning(const char *component, const char *format, ...)
{
    const char *c = component != NULL ? component : default_component;

    _cpl_msg_init(cpl_func);

    if (format == NULL) {
        cpl_msg_warning(c, default_format);
    } else {
        va_list     al;
        va_start(al, format);
        cpl_msg_out(CPL_MSG_WARNING, c, 0, format, al);
        va_end(al);
    }
}


/**
 * @brief
 *   Display an information message.
 *
 * @return Nothing.
 *
 * @param component  Name of the function generating the message.
 * @param format     Format string.
 * @param ...        Variable argument list associated to the format string.
 *
 * See the description of the function @c cpl_msg_error().
 */

void cpl_msg_info(const char *component, const char *format, ...)
{
    const char *c = component != NULL ? component : default_component;

    _cpl_msg_init(cpl_func);

    if (format == NULL) {
        cpl_msg_info(c, default_format);
    } else {
        va_list     al;
        va_start(al, format);
        cpl_msg_out(CPL_MSG_INFO, c, 0, format, al);
        va_end(al);
    }
}


/**
 * @brief
 *   Display an overwritable information message.
 *
 * @return Nothing.
 *
 * @param component  Name of the function generating the message.
 * @param format     Format string.
 * @param ...        Variable argument list associated to the format string.
 *
 * See the description of the function @c cpl_msg_error(). The severity
 * of the message issued by @c cpl_msg_info_overwritable() is the same
 * as the severity of a message issued using @c cpl_msg_info(). The only
 * difference with the @c cpl_msg_info() function is that the printed
 * message would be overwritten by a new message issued using again
 * cpl_msg_info_overwritable(), if no other meassages were issued with
 * other messaging functions in between. This function would be used
 * typically in loops, as in the following example:
 * @code
 *  iter = 1000;
 *  for (i = 0; i < iter; i++) {
 *      cpl_msg_info_overwritable(cpl_func,
                       "Median computation... %d out of %d", i, iter);
 *      <median computation would take place here>
 *  }
 * @endcode
 *
 * @note
 *   In the current implementation, an overwritable message is obtained
 *   by not adding the new-line character ('\\n') at the end of the message
 *   (contrary to what @c cpl_msg_info() does).
 */

void cpl_msg_info_overwritable(const char *component, const char *format, ...)
{
    const char *c = component != NULL ? component : default_component;

    _cpl_msg_init(cpl_func);

    overwrite = 1;

    if (format == NULL) {
        cpl_msg_info(c, default_format);
    } else {
        va_list     al;
        va_start(al, format);
        cpl_msg_out(CPL_MSG_INFO, c, 1, format, al);
        va_end(al);
    }
}


/**
 * @brief
 *   Display a debug message.
 *
 * @return Nothing.
 *
 * @param component  Name of the function generating the message.
 * @param format     Format string.
 * @param ...        Variable argument list associated to the format string.
 *
 * See the description of the function @c cpl_msg_error().
 */

void cpl_msg_debug(const char *component, const char *format, ...)
{
    const char *c = component != NULL ? component : default_component;

    _cpl_msg_init(cpl_func);

    if (format == NULL) {
        cpl_msg_debug(c, default_format);
    } else {
        va_list     al;
        va_start(al, format);
        cpl_msg_out(CPL_MSG_DEBUG, c, 0, format, al);
        va_end(al);
    }
}


/**
 * @brief
 *   Display a progress message predicting the time required in a loop.
 *
 * @return Nothing.
 *
 * @param component  Name of the function generating the message.
 * @param i          Iteration, starting with 0 and less than iter.
 * @param iter       Total number of iterations.
 * @param format     Format string.
 * @param ...        Variable argument list associated to the format string.
 * @see cpl_msg_info()
 * @deprecated       Use standard calls such as cpl_msg_info() instead.
 *
 */

void cpl_msg_progress(const char *component, int i, int iter,
                      const char *format, ...)
{

    const char *c = component != NULL ? component : default_component;

    const  double tquiet = 10.0; /* Accept silence for this many seconds */
    static double tstart =  0.0; /* Initialize to avoid false warnings */
    static double tend   =  0.0; /* Initialize to avoid false warnings */
    double tspent;
    static int iprev = 0;      /* Used to detect some illegal calls */
    static int nprev = 0;      /* Used to detect some illegal calls */
    static int didmsg = 0;

    int percent;


    _cpl_msg_init(cpl_func);

    if (format == NULL) format = default_format;

    if (i >= iter || i < 0 || iter < 1)
        return;

    if (i == 0) {
        if (iter == 1) return; /* A meaningful message is not possible */
        /* Reset check variables */
        nprev = iter;
        iprev = 0;
        /* Assume caller printed a message before the loop started */
        /* Find out how much CPU was spent at this point */
        tstart = cpl_test_get_cputime();
        tend = 0;
        didmsg = 0;
        return;
    }

    /* More input errors: i must increase during loop */
    if (i <= iprev) return; /* cpl_ensure() ? */
    iprev = i;

    /* More input errors: iter may not change during loop */
    if (iter != nprev) return; /* cpl_ensure() ? */

    /* Compute the time spent in the loop so far */
    tspent = cpl_test_get_cputime() - tstart;

    /* This fraction (rounded down) of iterations has been completed */
    percent = (i * 100) / iter;

    if (i == iter-1 && didmsg)
        cpl_msg_debug(c, "Loop time prediction offset (%d%% done) [s]: "
                      "%.2g", percent, tend - tspent);

    /* Return if the time spent is within the allowed time of silence
       + the predicted time if any */
    if (tspent < tquiet + tend) return;

    /* A prediction has not (yet) been made, or
       the prediction was too optismistic.

       Make a (new) prediction on the assumption that the average time
       so far required per iteration is unchanged for the remaining
       iteration(s). */

    tend =  tspent * (iter - i) / (double) i;

    /* Update the starting point for the prediction */
    tstart += tspent;

    if (tend >= 0.5) {  /* Do not predict less than 1 second */

        const int itend = 0.5 + tend; /* Roundoff to integer */

        /* %% is expanded twice */
        char * extformat = cpl_sprintf("%s. %d%%%% done, about %d seconds left",
                                       format, percent, itend);
        va_list     al;

        va_start(al, format);
        cpl_msg_out(CPL_MSG_INFO, c, 0, extformat, al);
        va_end(al);
        didmsg++;

        cpl_free(extformat);
    }

}

/**@}*/
