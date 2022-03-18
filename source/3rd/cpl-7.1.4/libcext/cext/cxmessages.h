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

#ifndef CX_MESSAGES_H
#define CX_MESSAGES_H

#include <stdarg.h>

#include <cxmacros.h>
#include <cxtypes.h>


CX_BEGIN_DECLS

/*
 * Message level offset for user defined message levels
 * (0 - 7 are used internally).
 */

#define CX_LOG_LEVEL_USER_SHIFT  (8)


/*
 * Log levels and flags
 */

typedef enum
{
    /* flags */
    CX_LOG_FLAG_RECURSION = 1 << 0,
    CX_LOG_FLAG_FATAL     = 1 << 1,

    /* levels */
    CX_LOG_LEVEL_ERROR    = 1 << 2,
    CX_LOG_LEVEL_CRITICAL = 1 << 3,
    CX_LOG_LEVEL_WARNING  = 1 << 4,
    CX_LOG_LEVEL_MESSAGE  = 1 << 5,
    CX_LOG_LEVEL_INFO     = 1 << 6,
    CX_LOG_LEVEL_DEBUG    = 1 << 7,

    CX_LOG_LEVEL_MASK     = ~(CX_LOG_FLAG_RECURSION | CX_LOG_FLAG_FATAL)
} cx_log_level_flags;

#define CX_LOG_FATAL_MASK (CX_LOG_FLAG_RECURSION | CX_LOG_LEVEL_ERROR)


/*
 * Message handlers
 */

typedef void (*cx_log_func) (const cxchar *, cx_log_level_flags,
                                const cxchar *, cxptr);
typedef void (*cx_print_func) (const cxchar *);


/*
 * Messaging mechanisms
 */

void cx_log_default_handler(const cxchar *, cx_log_level_flags,
                            const cxchar *, cxptr);
cx_log_func cx_log_set_default_handler(cx_log_func);
cxuint cx_log_set_handler(const cxchar *, cx_log_level_flags,
                          cx_log_func, cxptr);
void cx_log_remove_handler(const cxchar *, cxuint);

cx_log_level_flags cx_log_set_fatal_mask(const cxchar *, cx_log_level_flags);
cx_log_level_flags cx_log_set_always_fatal(cx_log_level_flags);

cxsize cx_log_get_domain_count(void);
const cxchar *cx_log_get_domain_name(cxsize);

void cx_log(const cxchar *, cx_log_level_flags,
            const cxchar *, ...) CX_GNUC_PRINTF(3, 4);
void cx_logv(const cxchar *, cx_log_level_flags,
             const cxchar *, va_list) CX_GNUC_PRINTF(3, 0);

cx_print_func cx_print_set_handler(cx_print_func);
cx_print_func cx_printerr_set_handler(cx_print_func);

void cx_print(const cxchar *, ...) CX_GNUC_PRINTF(1, 2);
void cx_printerr(const cxchar *, ...) CX_GNUC_PRINTF(1, 0);


/*
 * Convenience functions
 */

void cx_error(const cxchar *, ...) CX_GNUC_PRINTF(1, 2);
void cx_critical(const cxchar *, ...) CX_GNUC_PRINTF(1, 2);
void cx_warning(const cxchar *, ...) CX_GNUC_PRINTF(1, 2);
void cx_message(const cxchar *, ...) CX_GNUC_PRINTF(1, 2);


#ifndef CX_LOG_DOMAIN
#  define CX_LOG_DOMAIN  ((cxchar *)0)
#endif


/*
 * Macros for error handling.
 */

#ifdef CX_DISABLE_ASSERT

#  define cx_assert(expr)  /* empty */

#else /* !CX_DISABLE_ASSERT */

#  ifdef __GNUC__
#    define cx_assert(expr)                                           \
     do {                                                             \
         if (expr) {                                                  \
             ;                                                        \
         }                                                            \
         else {                                                       \
             cx_log(CX_LOG_DOMAIN, CX_LOG_LEVEL_ERROR,                \
                    "file %s: line %d (%s): assertion failed: (%s)",  \
                    __FILE__, __LINE__, __PRETTY_FUNCTION__, #expr);  \
         }                                                            \
     } while (0)
#  else /* !__GNUC__ */
#    define cx_assert(expr)                                           \
     do {                                                             \
         if (expr) {                                                  \
             ;                                                        \
         }                                                            \
         else {                                                       \
             cx_log(CX_LOG_DOMAIN,CX_LOG_LEVEL_ERROR,                 \
                    "file %s: line %d: assertion failed: (%s)",       \
                    __FILE__, __LINE__, #expr);                       \
         }                                                            \
     } while (0)
#  endif /* !__GNUC__ */

#endif /* !CX_DISABLE_ASSERT */

CX_END_DECLS

#endif /* CX_MESSAGES_H */

