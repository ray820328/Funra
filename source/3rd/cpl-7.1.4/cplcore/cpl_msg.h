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

#ifndef CPL_MSG_H
#define CPL_MSG_H

#include <cpl_macros.h>
#include <cpl_type.h>
#include <cpl_error.h>

CPL_BEGIN_DECLS

/**
 * @ingroup cpl_messaging
 * @brief 
 *   Messaging verbosity
 *
 * Messages may be printed with any of the functions @c cpl_msg_debug(),
 * @c cpl_msg_info(), @c cpl_msg_warning() and @c cpl_msg_error(). Choosing
 * one of these functions means to assign a level of severity to a given
 * message. The messaging system can then be set to display just messages 
 * having a high enough severity. The highest verbosity level of the 
 * messaging system is @c CPL_MSG_DEBUG, that would ensure that @em all 
 * the messages would be printed. The verbosity would progressively 
 * decrease through the levels @c CPL_MSG_INFO, @c CPL_MSG_WARNING, and 
 * @c CPL_MSG_ERROR, where only messages served by the @c cpl_msg_error() 
 * function would be printed. The lowest verbosity level, @c CPL_MSG_OFF, 
 * would inhibit the printing of any message to the terminal.
 */

#ifndef CPL_MAX_MSG_LENGTH
#define CPL_MAX_MSG_LENGTH      1024
#endif
#ifndef CPL_MAX_FUNCTION_NAME
#define CPL_MAX_FUNCTION_NAME   50
#endif
#ifndef CPL_MAX_DOMAIN_NAME
#define CPL_MAX_DOMAIN_NAME     40
#endif
#ifndef CPL_MAX_LOGFILE_NAME
#define CPL_MAX_LOGFILE_NAME    72
#endif

enum _cpl_msg_severity_ {
    CPL_MSG_DEBUG   = 0,
    CPL_MSG_INFO,
    CPL_MSG_WARNING,
    CPL_MSG_ERROR,
    CPL_MSG_OFF
};

typedef enum _cpl_msg_severity_ cpl_msg_severity;

cpl_error_code cpl_msg_init(void);
void cpl_msg_stop(void);

cpl_error_code cpl_msg_set_log_level(cpl_msg_severity);
cpl_error_code cpl_msg_stop_log(void);
const char *cpl_msg_get_log_name(void);
cpl_error_code cpl_msg_set_log_name(const char *);
void cpl_msg_set_level(cpl_msg_severity);
void cpl_msg_set_level_from_env(void);
cpl_msg_severity cpl_msg_get_log_level(void);
cpl_msg_severity cpl_msg_get_level(void);

void cpl_msg_set_time_on(void);
void cpl_msg_set_time_off(void);
void cpl_msg_set_component_on(void);
void cpl_msg_set_component_off(void);
void cpl_msg_set_domain_on(void);
void cpl_msg_set_domain_off(void);
void cpl_msg_set_threadid_on(void);
void cpl_msg_set_threadid_off(void);

void cpl_msg_set_domain(const char *);
const char *cpl_msg_get_domain(void);

void cpl_msg_set_width(int);
void cpl_msg_set_indentation(int);
void cpl_msg_indent_more(void);
void cpl_msg_indent_less(void);
void cpl_msg_indent(int);

void cpl_msg_error(const char *, const char *, ...)   CPL_ATTR_PRINTF(2,3);
void cpl_msg_warning(const char *, const char *, ...) CPL_ATTR_PRINTF(2,3);
void cpl_msg_info(const char *, const char *, ...)    CPL_ATTR_PRINTF(2,3);
void cpl_msg_debug(const char *, const char *, ...)   CPL_ATTR_PRINTF(2,3);
void cpl_msg_info_overwritable(const char *,
                               const char *, ...)     CPL_ATTR_PRINTF(2,3);
void cpl_msg_progress(const char *,  int, int, const char *,
                      ...) CPL_ATTR_DEPRECATED CPL_ATTR_PRINTF(4,5);

CPL_END_DECLS

#endif /* end of cpl_messages.h */
