/*
 * This file is part of the ESO C Extension Library
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

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifdef HAVE_CHAR_BIT
#  include <limits.h>
#endif

#include "cxthread.h"
#include "cxmemory.h"
#include "cxstring.h"
#include "cxmessages.h"
#include "cxstrutils.h"
#include "cxutils.h"


#define CX_MSG_FORMAT_UNSIGNED_BUFFER_SIZE  ((3 * SIZEOF_LONG) + 3)
#define CX_MSG_PREFIX_BUFFER_SIZE           \
    (CX_MSG_FORMAT_UNSIGNED_BUFFER_SIZE + 32)
#define CX_MSG_ALERT_LEVELS                 \
    (CX_LOG_LEVEL_ERROR | CX_LOG_LEVEL_CRITICAL | CX_LOG_LEVEL_WARNING)



/**
 * @defgroup cxmessages Message Logging
 *
 * The module implements a flexible logging facility. It can be customized
 * to fit into the application environment. Log levels and functions can be
 * defined and used in addition to or as replacement of the built in levels
 * and log functions.
 *
 * @par Synopsis:
 * @code
 *   #include <cxmessages.h>
 * @endcode
 */

/**@{*/

typedef cxint cx_file_descriptor;


/*
 * Log handler and log domain structures.
 */

typedef struct _cx_log_domain_ cx_log_domain;
typedef struct _cx_log_handler_ cx_log_handler;

struct _cx_log_handler_ {
    cxuint id;
    cx_log_level_flags log_level;
    cx_log_func log_func;
    cxptr data;
    cx_log_handler *next;
};

struct _cx_log_domain_ {
    cxchar *name;
    cx_log_level_flags fatal_mask;
    cx_log_handler *handlers;
    cx_log_domain *next;
};


/*
 * Messaging system mutexes
 */

CX_LOCK_DEFINE_STATIC(cx_messages_lock);

CX_ONCE_DEFINE_INITIALIZED_STATIC(cx_messages_once);


/*
 * Built in system defaults
 */

static cx_log_domain *cx_log_domains = NULL;
static cx_log_level_flags cx_log_always_fatal = CX_LOG_FATAL_MASK;
static cx_log_func cx_log_handler_func = cx_log_default_handler;
static cx_print_func cx_print_message_printer = NULL;
static cx_print_func cx_print_error_printer = NULL;

CX_PRIVATE_DEFINE_STATIC(cx_log_depth);

static cx_log_level_flags cx_log_prefix = (CX_LOG_LEVEL_ERROR    |
                                           CX_LOG_LEVEL_WARNING  |
                                           CX_LOG_LEVEL_CRITICAL |
                                           CX_LOG_LEVEL_DEBUG);


/*
 * Library debugging system
 */

typedef struct _cx_debug_key_ cx_debug_key;

struct _cx_debug_key_ {
    const cxchar *key;
    cxuint value;
};

typedef enum {
    CX_DEBUG_FATAL_WARNINGS  = 1 << 0,
    CX_DEBUG_FATAL_CRITICALS = 1 << 1
} cx_debug_flag;

static cxbool cx_debug_initialized = FALSE;
static cxuint cx_debug_flags = 0;

static cxuint
cx_debug_parse_string(const cxchar *string, const cx_debug_key *keys,
                      cxuint nkeys)
{

    cxuint i;
    cxuint result = 0;


    if (string == 0)
        return 0;

    if (!cx_strcasecmp(string, "all")) {
        for (i = 0; i < nkeys; i++)
            result |= keys[i].value;
    }
    else {

        const cxchar *p = string;
        const cxchar *q;
        cxbool done = FALSE;

        while (*p && !done) {
            q = strchr(p, ':');

            if (!q) {
                q = p + strlen(p);
                done = TRUE;
            }

            for (i = 0; i < nkeys; i++) {
                if (cx_strncasecmp(keys[i].key, p, q - p) == 0 &&
                    keys[i].key[q - p] == '\0')
                    result |= keys[i].value;
            }
        }
    }

    return result;

}


static void
cx_debug_init(void)
{

    const cxchar *value;


    cx_debug_initialized = TRUE;

    value = getenv("CX_DEBUG");
    if (value != NULL) {
        static const cx_debug_key keys[] = {
            {"fatal_warnings", CX_DEBUG_FATAL_WARNINGS},
            {"fatal_criticals", CX_DEBUG_FATAL_CRITICALS}
        };

        cx_debug_flags = cx_debug_parse_string(value, keys,
                                               CX_N_ELEMENTS(keys));
    }

    if (cx_debug_flags & CX_DEBUG_FATAL_WARNINGS) {

        cx_log_level_flags fatal_mask;

        fatal_mask = cx_log_set_always_fatal(CX_LOG_FATAL_MASK);
        fatal_mask |= CX_LOG_LEVEL_WARNING | CX_LOG_LEVEL_CRITICAL;

        cx_log_set_always_fatal(fatal_mask);

    }

    if (cx_debug_flags & CX_DEBUG_FATAL_CRITICALS) {

        cx_log_level_flags fatal_mask;

        fatal_mask = cx_log_set_always_fatal(CX_LOG_FATAL_MASK);
        fatal_mask |= CX_LOG_LEVEL_CRITICAL;

        cx_log_set_always_fatal(fatal_mask);

    }

    return;

}


/*
 * Write a string to a file descriptor.
 */

inline static cxssize
cx_msg_write_string(cx_file_descriptor fd, const cxchar *string)
{
    return write(fd, string, strlen(string));
}


/*
 * Write an unsigned integer value as formatted string to a file
 * descriptor. The format is determined by the given radix, which
 * has to be in the range 2 <= radix <= 36.
 */

inline static void
cx_msg_format_unsigned(cxchar *buffer, cxulong value, cxuint radix)
{

    cxchar c;
    cxint i, n;
    cxulong tmp;


    /* Do not use any cext library functions here! */

    if ((radix != 8) && (radix != 10)  && (radix !=16)) {
        *buffer = '\0';
        return;
    }

    if (!value) {
        *buffer++ = '0';
        *buffer   = '\0';
        return;
    }


    /* Hexadecimal or octal prefix for radix 16 or 8. */

    if (radix == 16) {
        *buffer++ = '0';
        *buffer++ = 'x';
    }
    else if (radix == 8)
        *buffer++ = '0';


    /* Number of digits needed. */

    n = 0;
    tmp = value;

    while (tmp) {
        tmp /= radix;
        ++n;
    }


    /* Fill buffer with character representation */

    i = n;

    if (n > CX_MSG_FORMAT_UNSIGNED_BUFFER_SIZE - 3) {
        *buffer = '\0';
        return;
    }

    while (value) {
        --i;
        c = (value % radix);

        if (c < 10)
            buffer[i] = c + '0';
        else
            buffer[i] = c + 'a' - 10;

        value /= radix;
    }

    buffer[n] = '\0';

    return;

}


/*
 * Fill a buffer with the formatted log message prefix. The return value is
 * the target file descriptor of the message.
 */

inline static cx_file_descriptor
cx_msg_format_prefix(cxchar prefix[], cx_log_level_flags level)
{

    cxbool is_normal = TRUE;


    /* Do not use any cext library functions here! */

    switch (level & CX_LOG_LEVEL_MASK) {
        case CX_LOG_LEVEL_ERROR:
            strcpy(prefix, "ERROR");
            is_normal = FALSE;
            break;

        case CX_LOG_LEVEL_CRITICAL:
            strcpy(prefix, "CRITICAL");
            is_normal = FALSE;
            break;

        case CX_LOG_LEVEL_WARNING:
            strcpy(prefix, "WARNING");
            is_normal = FALSE;
            break;

        case CX_LOG_LEVEL_MESSAGE:
            strcpy(prefix, "Message");
            is_normal = FALSE;
            break;

        case CX_LOG_LEVEL_INFO:
            strcpy(prefix, "INFO");
            break;

        case CX_LOG_LEVEL_DEBUG:
            strcpy(prefix, "DEBUG");
            break;

        default:
            if (level) {
                strcpy(prefix, "LOG-");
                cx_msg_format_unsigned(prefix + 4, level & CX_LOG_LEVEL_MASK,
                                       16);
            }
            else
                strcpy(prefix, "LOG");
            break;
    }

    if (level & CX_LOG_FLAG_RECURSION)
        strcat(prefix, " (recursed)");

    if (level & CX_MSG_ALERT_LEVELS)
        strcat(prefix, " **");

    return is_normal ? 1 : 2;

}


/*
 * Initialize the log message prefix
 */

inline static void
cx_log_prefix_init(void)
{

    static cxbool initialized = FALSE;


    if (!initialized) {

        const cxchar *value;

        initialized = TRUE;
        value = getenv("CX_MESSAGES_PREFIXED");

        if (value) {
            static const cx_debug_key keys[] = {
                {"error", CX_LOG_LEVEL_ERROR},
                {"critical", CX_LOG_LEVEL_CRITICAL},
                {"warning", CX_LOG_LEVEL_WARNING},
                {"message", CX_LOG_LEVEL_MESSAGE},
                {"info", CX_LOG_LEVEL_INFO},
                {"debug", CX_LOG_LEVEL_DEBUG}
            };

            cx_log_prefix = cx_debug_parse_string(value, keys,
                                                  CX_N_ELEMENTS(keys));
        }
    }

    return;

}


/*
 * Last resort print handler
 */

static void
cx_log_fallback_handler(const cxchar *name, cx_log_level_flags level,
                        const cxchar *message, cxptr data)
{

    cxchar prefix[CX_MSG_PREFIX_BUFFER_SIZE];
    cxchar strpid[CX_MSG_FORMAT_UNSIGNED_BUFFER_SIZE];

    cxbool is_fatal = (level & CX_LOG_FLAG_FATAL) != 0;

    cx_file_descriptor fd = cx_msg_format_prefix(prefix, level);


    /* Keep complier quiet */
    (void) data;

    if (!message)
        message = "(NULL) message";

    cx_msg_format_unsigned(strpid, getpid(), 10);

    if (name)
        cx_msg_write_string(fd, "\n");
    else
        cx_msg_write_string(fd, "\n**");

    cx_msg_write_string(fd, "(process:");
    cx_msg_write_string(fd, strpid);
    cx_msg_write_string(fd, "): ");

    if (name) {
        cx_msg_write_string(fd, name);
        cx_msg_write_string(fd, "-");
    }

    cx_msg_write_string(fd, prefix);
    cx_msg_write_string(fd, ": ");
    cx_msg_write_string(fd, message);

    if (is_fatal)
        cx_msg_write_string(fd, "\naborting...\n");
    else
        cx_msg_write_string(fd, "\n");

    return;

}


/*
 * Create a new domain.
 */

inline static cx_log_domain *
cx_log_domain_new(const cxchar *domain_name)
{

    register cx_log_domain *domain;


    domain = cx_malloc(sizeof *domain);
    domain->name = cx_strdup(domain_name);
    domain->fatal_mask = CX_LOG_FATAL_MASK;
    domain->handlers = NULL;

    domain->next = cx_log_domains;
    cx_log_domains = domain;

    return domain;

}


/*
 * Log domain garbage collector
 */

inline static void
cx_log_domain_cleanup(cx_log_domain *domain)
{

    if (domain->fatal_mask == CX_LOG_FATAL_MASK && domain->handlers == NULL) {

        register cx_log_domain *last, *current;

        last = NULL;
        current = cx_log_domains;

        while (current) {
            if (current == domain) {
                if (last)
                    last->next = domain->next;
                else
                    cx_log_domains = domain->next;

                cx_free(domain->name);
                cx_free(domain);
                break;
            }

            last = current;
            current = last->next;
        }
    }

    return;

}


/*
 * Lookup a log domain
 */

inline static cx_log_domain *
cx_log_domain_find(const cxchar *log_domain)
{

    register cx_log_domain *domain;

    domain = cx_log_domains;
    while (domain) {
        if (strcmp(domain->name, log_domain) == 0)
            return domain;

        domain = domain->next;
    }

    return NULL;
}


/*
 * Get a log domains handler for the given log level
 */

inline static cx_log_func
cx_log_domain_get_handler(cx_log_domain *domain, cx_log_level_flags level,
                          cxptr *data)
{

    if (domain && level) {

        register cx_log_handler *handler;

        handler = domain->handlers;
        while (handler) {
            if ((handler->log_level & level) == level) {
                *data = handler->data;
                return handler->log_func;
            }

            handler = handler->next;
        }
    }

    return cx_log_handler_func;

}

/*
 * Message system initialization function for threaded environments.
 * For library internal use only!
 *
 * The current thread support does not need a initialization of the
 * thread environment. If this should ever be needed this function should
 * be called from the thread initialization function. This means that
 * This function has to be become a global function. To restrict the
 * visibility of the function to the library itself add the attribute
 * macro CX_GNUC_INTERNAL to the function declaration.
 */

static void
_cx_log_thread_init(void)
{

    CX_INITLOCK(cx_messages_lock, CX_MUTEX_TYPE_RECURSIVE);

    cx_private_init(cx_log_depth, NULL);
    cx_log_prefix_init();
    cx_debug_init();

    return;

}


/**
 * @brief
 *   Set log levels to be always fatal.
 *
 * @param mask  Log message level flags.
 *
 * @return Previous mask.
 *
 * Log levels set in the log message level flags mask @em mask will always be
 * treated as fatal. This applies only to the internally known log levels.
 * User defined log levels are not taken into account.
 *
 * In any case, the function forces errors to be fatal even if the error
 * level was not set in @em mask. The priviously set mask is replaced by
 * @em mask and is passed back to the caller as the return value.
 */

cx_log_level_flags
cx_log_set_always_fatal(cx_log_level_flags mask)
{

    cx_log_level_flags previous;


    /*
     * Restrict the global mask to internally known levels, force
     * errors to be fatal and remove a bogus flag
     */

    mask &= (1 << CX_LOG_LEVEL_USER_SHIFT) - 1;
    mask |= CX_LOG_LEVEL_ERROR;
    mask &= ~CX_LOG_FLAG_FATAL;

    CX_LOCK(cx_messages_lock);
    previous = cx_log_always_fatal;
    cx_log_always_fatal = mask;
    CX_UNLOCK(cx_messages_lock);

    return previous;

}


/**
 * @brief
 *   Get the number of registered log domains.
 *
 * @return The number of currently registered log domains.
 *
 * The function counts the registered log domains and returns the
 * total number of log domains. The returned number may be 0 if no
 * log domain was previously registered.
 */

cxsize
cx_log_get_domain_count(void)
{

    register cxsize count = 0;
    register cx_log_domain *domain = cx_log_domains;


    while (domain) {
        ++count;
        domain = domain->next;
    }

    return count;

}


/**
 * @brief
 *   Get the name of a log domain.
 *
 * @param position  Index of the log domain to lookup.
 *
 * @return The function returns the name of the log domain, or @c NULL
 *   if @em position is out of range.
 *
 * The function retrieves the name of the log domain registered at index
 * position @em position. The valid range for @em position is from 0 to 1
 * less than the number of domains registered. If an invalid log domain is
 * requested, i.e. no log domain has been previously registered for the given
 * position, the function returns @c NULL.
 *
 * @see cx_log_get_domain_count()
 */

const cxchar *
cx_log_get_domain_name(cxsize position)
{

    register cx_log_domain *domain = cx_log_domains;


    while (domain) {
        if (position == 0) {
            return domain->name;
        }
        --position;
        domain = domain->next;
    }

    return NULL;

}


/**
 * @brief
 *   Sets the log message level which are fatal for a given domain.
 *
 * @param name        Name of the log domain.
 * @param fatal_mask  The log domains new fatal mask.
 *
 * @return Previously installed fatal mask for the domain.
 *
 * The log message levels set in the flag mask @em fatal_mask are treated
 * as being fatal for the log domain with the name @em name. Even if the
 * error level is not set in @em fatal_mask the function forces errors to
 * be fatal.
 */

cx_log_level_flags
cx_log_set_fatal_mask(const cxchar *name, cx_log_level_flags fatal_mask)
{

    cx_log_level_flags previous;
    register cx_log_domain *domain;


    if (!name)
        name = "";


    /* Force errors to be fatal. Remove bogus flag. */

    fatal_mask |= CX_LOG_LEVEL_ERROR;
    fatal_mask &= ~CX_LOG_FLAG_FATAL;

    CX_LOCK(cx_messages_lock);

    domain = cx_log_domain_find(name);
    if (!domain)
        domain = cx_log_domain_new(name);

    previous = domain->fatal_mask;
    domain->fatal_mask = fatal_mask;
    cx_log_domain_cleanup(domain);

    CX_UNLOCK(cx_messages_lock);

    return previous;

}


/**
 * @brief
 *   Set the default log handler.
 *
 * @param func New handler function.
 *
 * @return The previously set print handler.
 *
 * The function @em func is installed as the new default log handler function.
 * Any message passed to @b cx_log() or @b cx_logv() is printed using this
 * handler unless a domain and level specific handler has been set for the
 * current message.
 *
 * @see cx_log_set_handler()
 */

cx_log_func
cx_log_set_default_handler(cx_log_func func)
{

    cx_log_func previous;


    CX_LOCK(cx_messages_lock);

    previous = cx_log_handler_func;
    cx_log_handler_func = func;

    CX_UNLOCK(cx_messages_lock);

    return previous;

}


/**
 * @brief
 *   Set the log handler for a log domain.
 *
 * @param name    Name of the log domain.
 * @param levels  Log levels.
 * @param func    log function.
 * @param data    User data.
 *
 * @return The handler's id for this domain.
 *
 * The log function @em func is set for the domain with the name @em name,
 * applicable for the log levels given by the flag mask @em levels. If
 * the log function @em func requires extra data this can be passed to
 * @em func through the user data @em data.
 */

cxuint cx_log_set_handler(const cxchar *name, cx_log_level_flags levels,
                          cx_log_func func, cxptr data)
{

    register cx_log_domain *domain;
    register cx_log_handler *handler;

    static cxuint handler_id = 0;


    if ((levels & CX_LOG_LEVEL_MASK) == 0)
        return 0;

    if (func == NULL)
        return 0;

    if (!name)
        name = "";

    CX_LOCK(cx_messages_lock);

    domain = cx_log_domain_find(name);
    if (!domain)
        domain = cx_log_domain_new(name);

    handler = cx_malloc(sizeof *handler);
    handler->id = ++handler_id;
    handler->log_level = levels;
    handler->log_func = func;
    handler->data = data;
    handler->next = domain->handlers;
    domain->handlers = handler;

    CX_UNLOCK(cx_messages_lock);

    return handler_id;

}


/**
 * @brief
 *   Remove a log handler from a domain.
 *
 * @param name  Name of the log domain.
 * @param id    Id number of the handler.
 *
 * Removes the log handler, i.e. in principle the log function, registered
 * with the id number @em id, from the list of log handlers for the log
 * domain @em name.
 */

void
cx_log_remove_handler(const cxchar *name, cxuint id)
{

    register cx_log_domain *domain;


    if (id == 0)
        return;

    if (!name)
        name = "";

    CX_LOCK(cx_messages_lock);

    domain = cx_log_domain_find(name);
    if (domain) {

        register cx_log_handler *last, *current;

        last = NULL;
        current = domain->handlers;

        while (current) {
            if (current->id == id) {
                if (last)
                    last->next = current->next;
                else
                    domain->handlers = current->next;

                cx_log_domain_cleanup(domain);
                CX_UNLOCK(cx_messages_lock);

                cx_free(current);

                return;
            }

            last = current;
            current = last->next;
        }
    }

    CX_UNLOCK(cx_messages_lock);

    cx_warning("cx_log_remove_handler(): could not find handler with "
               "id `%d' for domain \"%s\"", id, name);

    return;

}


/**
 * @brief
 *   Log a formatted message using a variable-length argument.
 *
 * @param name    Name of the log domain.
 * @param level   The message log level.
 * @param format  Format string defining output converstions.
 * @param args    Variable-length argument list.
 *
 * The log message, as defined by the format string @em format and arguments
 * given by the variable-length argument @em args is formatted according to
 * the conversion directives present in the format string. All standard
 * @b printf() conversion directives are supported.
 *
 * The formatted message is logged for the level @em level, if it is enabled,
 * using the log function set for the log domain @em name.
 */

void
cx_logv(const cxchar *name, cx_log_level_flags level, const cxchar *format,
        va_list args)
{

    cxbool is_fatal = (level & CX_LOG_FLAG_FATAL) != 0;
    cxbool in_recursion = (level & CX_LOG_FLAG_RECURSION) != 0;

    register cxint i;


    level &= CX_LOG_LEVEL_MASK;
    if (!level)
        return;


    /*
     * Logging system initialization. Takes place only once.
     */

    cx_thread_once(cx_messages_once, _cx_log_thread_init, NULL);


    for (i = cx_bits_find(level, -1); i >= 0; i = cx_bits_find(level, i)) {

        register cx_log_level_flags test_level;


        test_level = 1 << i;

        if (level & test_level) {

            cxuint depth = CX_POINTER_TO_UINT(cx_private_get(cx_log_depth));

            cx_log_level_flags fatal_mask;
            cx_log_domain *domain;
            cx_log_func log_func;

            cxptr data = NULL;


            if (is_fatal)
                test_level |= CX_LOG_FLAG_FATAL;

            if (in_recursion)
                test_level |= CX_LOG_FLAG_RECURSION;


            /*
             * Lookup handler
             */

            CX_LOCK(cx_messages_lock);

            domain = cx_log_domain_find(name ? name : "");

            if (depth)
                test_level |= CX_LOG_FLAG_RECURSION;

            ++depth;

            fatal_mask = domain ? domain->fatal_mask : CX_LOG_FATAL_MASK;

            if (((fatal_mask | cx_log_always_fatal) & test_level) != 0)
                test_level |= CX_LOG_FLAG_FATAL;

            if (test_level & CX_LOG_FLAG_RECURSION)
                log_func = cx_log_fallback_handler;
            else
                log_func = cx_log_domain_get_handler(domain, test_level, &data);

            domain = NULL;

            CX_UNLOCK(cx_messages_lock);

            cx_private_set(cx_log_depth, CX_UINT_TO_POINTER(depth));


            /*
             * Debug initialization
             */

            if (!(test_level & CX_LOG_FLAG_RECURSION) &&
                    !cx_debug_initialized) {

                cx_log_level_flags test_level_saved = test_level;


                cx_debug_init();

                if (((fatal_mask | cx_log_always_fatal) & test_level) != 0)
                    test_level |= CX_LOG_FLAG_FATAL;

                if (test_level != test_level_saved) {

                    CX_LOCK(cx_messages_lock);

                    domain = cx_log_domain_find(name ? name : "");
                    log_func = cx_log_domain_get_handler(domain, test_level,
                                                         &data);
                    domain = NULL;

                    CX_UNLOCK(cx_messages_lock);

                }

            }

            if (test_level & CX_LOG_FLAG_RECURSION) {

                /*
                 * Use fixed size stack buffer, because we might be out of
                 * memory.
                 */

                cxchar buffer[1025];
                va_list args2;


                CX_VA_COPY(args2, args);
                cx_vsnprintf(buffer, 1024, format, args);
                va_end(args2);

                log_func(name, test_level, buffer, data);

            }
            else {

                cxchar *buffer;
                va_list args2;

                CX_VA_COPY(args2, args);
                buffer = cx_strvdupf(format, args2);
                va_end(args2);

                log_func(name, test_level, buffer, data);

                cx_free(buffer);

            }

            if (test_level & CX_LOG_FLAG_FATAL)
                abort();

            --depth;
            cx_private_set(cx_log_depth, CX_UINT_TO_POINTER(depth));

        }
    }

    return;

}


/**
 * @brief
 *   Log a formatted message.
 *
 * @param name    Name of the log domain.
 * @param level   The message log level.
 * @param format  Format string defining output converstions.
 * @param ...     Argument list.
 *
 * The log message, as defined by the format string @em format and the
 * corresponding argument list is logged with the level @em level, if it
 * is enabled, using the log function set for the log domain @em name.
 * given by the variable-length argument is formatted according to the
 * All standard @b printf() conversion directives are supported.
 */

void
cx_log(const cxchar *name, cx_log_level_flags level, const cxchar *format, ...)
{

    va_list args;

    va_start(args, format);
    cx_logv(name, level, format, args);
    va_end(args);

    return;

}


/**
 * @brief
 *   Default log handler.
 *
 * @param name     The message's log domain name
 * @param level    Log level of the message
 * @param message  The message text
 * @param data     Extra data passed by the caller (ignored!)
 *
 * @return Nothing.
 *
 * The default log handler, which is used if no log handler has been set by a
 * call to @b cx_log_set_handler() for the combination domain @em name and
 * log level @em level. The message text @em message is written to @c stdout,
 * or @c stderr if the level is one of @c CX_LOG_LEVEL_ERROR,
 * @c CX_LOG_LEVEL_CRITICAL and @c CX_LOG_LEVEL_WARNING. In addition, if the
 * log level is fatal the program is aborted by a call to @b abort().
 *
 * @see cx_log_set_handler()
 */

void
cx_log_default_handler(const cxchar *name, cx_log_level_flags level,
                       const cxchar *message, cxptr data)
{

    cxchar prefix[CX_MSG_PREFIX_BUFFER_SIZE];
    cxbool is_fatal = (level & CX_LOG_FLAG_FATAL) != 0;

    cx_string* msg = NULL;

    cx_file_descriptor fd;


    if (level & CX_LOG_FLAG_RECURSION) {
        cx_log_fallback_handler(name, level, message, data);
        return;
    }

    cx_log_prefix_init();
    fd = cx_msg_format_prefix(prefix, level);

    msg = cx_string_new();

    if ((cx_log_prefix & level) == level) {

        const cxchar *program = cx_program_get_name();

        if (program)
            cx_string_sprintf(msg, "(%s:%lu): ", program, (cxulong)getpid());
        else
            cx_string_sprintf(msg, "(process:%lu): ", (cxulong)getpid());

    }

    if (!name)
        cx_string_prepend(msg, "** ");

    if (level & CX_MSG_ALERT_LEVELS)
        cx_string_prepend(msg, "\n");

    if (name) {
        cx_string_append(msg, name);
        cx_string_append(msg, "-");
    }

    cx_string_append(msg, prefix);
    cx_string_append(msg, ": ");

    if (!message)
        cx_string_append(msg, "(NULL) message");
    else
        cx_string_append(msg, message);

    if (is_fatal)
        cx_string_append(msg, "\naborting...\n");
    else
        cx_string_append(msg, "\n");

    cx_msg_write_string(fd, cx_string_get(msg));

    cx_string_delete(msg);

    return;

}


/**
 * @brief
 *   Set handler for message output.
 *
 * @param func New handler function.
 *
 * @return The previously set print handler.
 *
 * The function @em func is installed as the new message printing function.
 * Any message passed to @b cx_print() is printed using this handler. The
 * default print handler just outputs the message text to @c stdout.
 *
 * @see cx_print()
 */

cx_print_func
cx_print_set_handler(cx_print_func func)
{

    cx_print_func previous;


    CX_LOCK(cx_messages_lock);

    previous = cx_print_message_printer;
    cx_print_message_printer = func;

    CX_UNLOCK(cx_messages_lock);

    return previous;

}


/**
 * @brief
 *   Output a formatted message via the print handler.
 *
 * @param format  The message format.
 * @param ...     Argument list.
 *
 * @return Nothing.
 *
 * The output message created from the format string @em format and the
 * converted arguments from the argument list is output via the currently
 * set print handler. The format string may contain all conversion
 * directives supported by @b printf(). The default print handler outputs
 * messages to @c stdout.
 *
 * The @b cx_print() function should not be from within libraries for
 * debugging messages, since it may be redirected by applications. Instead,
 * libraries should use @b cx_log(), or the convenience functions
 * @b cx_error(), @b cx_critical(), @b cx_warning() and @b cx_message().
 *
 * @see cx_print_set_handler()
 */

void
cx_print(const cxchar *format, ...)
{

    va_list args;
    cxchar *string;
    cx_print_func printer;


    if (format == NULL)
        return;

    va_start(args, format);
    string = cx_strvdupf(format, args);
    va_end(args);

    CX_LOCK(cx_messages_lock);
    printer = cx_print_message_printer;
    CX_UNLOCK(cx_messages_lock);

    if (printer)
        printer(string);
    else {
        fputs(string, stdout);
        fflush(stdout);
    }

    cx_free(string);
    return;

}


/**
 * @brief
 *   Set handler for error message output.
 *
 * @param func New handler function.
 *
 * @return The previously set error message handler.
 *
 * The function @em func is installed as the new error message printing
 * function. Any message passed to @b cx_printerr() is printed using this
 * handler. The default print handler just outputs the error message text
 * to @c stderr.
 *
 * @see cx_printerr()
 */

cx_print_func
cx_printerr_set_handler(cx_print_func func)
{

    cx_print_func previous;


    CX_LOCK(cx_messages_lock);

    previous = cx_print_error_printer;
    cx_print_error_printer = func;

    CX_UNLOCK(cx_messages_lock);

    return previous;

}


/**
 * @brief
 *   Output a formatted message via the error message handler.
 *
 * @param format  The message format.
 * @param ...     Argument list.
 *
 * @return Nothing.
 *
 * The output error message created from the format string @em format and the
 * converted arguments from the argument list is output via the currently
 * set error message handler. The format string may contain all conversion
 * directives supported by @b printf(). The default error message handler
 * outputs messages to @c stderr.
 *
 * The @b cx_printerr() function should not be from within libraries for
 * debugging messages, since it may be redirected by applications. Instead,
 * libraries should use @b cx_log(), or the convenience functions
 * @b cx_error(), @b cx_critical(), @b cx_warning() and @b cx_message().
 *
 * @see cx_printerr_set_handler()
 */

void
cx_printerr(const cxchar *format, ...)
{

    va_list args;
    cxchar *string;
    cx_print_func printer;


    if (format == NULL)
        return;

    va_start(args, format);
    string = cx_strvdupf(format, args);
    va_end(args);

    CX_LOCK(cx_messages_lock);
    printer = cx_print_error_printer;
    CX_UNLOCK(cx_messages_lock);

    if (printer)
        printer(string);
    else {
        fputs(string, stderr);
        fflush(stderr);
    }

    cx_free(string);
    return;

}


/**
 * @brief
 *   Log an error message.
 *
 * @param format  The format string.
 * @param ...     Arguments to be inserted into the format string.
 *
 * @return Nothing.
 *
 * This is a convenience function to log an error message specified by the
 * format string @em format and the following list of arguments via the
 * installed log handler.
 *
 * Error messages are always considered fatal, i.e. the application is
 * immediately terminated by a call to @b abort() causing a core dump. Do
 * not use this function for expected (recoverable) errors.
 * This function should be used to indicate a bug (assertion failure) in the
 * application.
 *
 * @see cx_critical()
 */

void cx_error(const cxchar *format, ...)
{

    va_list args;

    va_start(args, format);
    cx_logv(CX_LOG_DOMAIN, CX_LOG_LEVEL_ERROR, format, args);
    va_end(args);

    return;
}


/**
 * @brief
 *   Log a "critical" warning.
 *
 * @param format  The format string.
 * @param ...     Arguments to be inserted into the format string.
 *
 * @return Nothing.
 *
 * This is a convenience function to log a message with level
 * @c CX_LOG_LEVEL_CRITICAL, as specified by the format string @em format and
 * the following list of arguments, via the installed log handler.
 *
 * It is up to the application to decide which warnings are critical and
 * which are not. To cause a termination of the application on critical
 * warnings you may call @b cx_log_set_always_fatal().
 *
 * @see cx_warning()
 */

void cx_critical(const cxchar *format, ...)
{
    va_list args;

    va_start(args, format);
    cx_logv(CX_LOG_DOMAIN, CX_LOG_LEVEL_CRITICAL, format, args);
    va_end(args);

    return;
}


/**
 * @brief
 *   Log a warning.
 *
 * @param format  The format string.
 * @param ...     Arguments to be inserted into the format string.
 *
 * @return Nothing.
 *
 * This is a convenience function to log a warning message, as specified
 * by the format string @em format and the following list of arguments,
 * via the installed log handler.
 *
 * @see cx_critical()
 */

void cx_warning(const cxchar *format, ...)
{
    va_list args;

    va_start(args, format);
    cx_logv(CX_LOG_DOMAIN, CX_LOG_LEVEL_WARNING, format, args);
    va_end(args);

    return;
}


/**
 * @brief
 *   Log a normal message.
 *
 * @param format  The format string.
 * @param ...     Arguments to be inserted into the format string.
 *
 * @return Nothing.
 *
 * This is a convenience function to log an ordinary message, as specified
 * by the format string @em format and the following list of arguments,
 * via the installed log handler.
 *
 */

void cx_message(const cxchar *format, ...)
{
    va_list args;

    va_start(args, format);
    cx_logv(CX_LOG_DOMAIN, CX_LOG_LEVEL_MESSAGE, format, args);
    va_end(args);

    return;
}
/**@}*/
