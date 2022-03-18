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

#ifndef CX_STRING_H_
#define CX_STRING_H_ 1

#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <cxtypes.h>
#include <cxmemory.h>
#include <cxmessages.h>
#include <cxutils.h>


CX_BEGIN_DECLS

struct _cx_string_ {

    /* <private> */

    cxchar  *data;
    cxsize sz;

};


/**
 * @ingroup cxstring
 *
 * @brief
 *   The cx_string data type.
 */

typedef struct _cx_string_ cx_string;


/*
 * Create, copy and destroy operations
 */

cx_string *cx_string_new(void);
cx_string *cx_string_copy(const cx_string *);
cx_string *cx_string_create(const cxchar *);
void cx_string_delete(cx_string *);

/*
 * Non modifying operations
 */

cxsize cx_string_size(const cx_string *);
cxbool cx_string_empty(const cx_string *);

/*
 * Data access
 */

const cxchar *cx_string_get(const cx_string *);

/*
 * Assignment operations
 */

void cx_string_set(cx_string *, const cxchar *);

/*
 * Modifying operations
 */
void cx_string_resize(cx_string *, cxsize, cxchar);
void cx_string_extend(cx_string *, cxsize, cxchar);

void cx_string_replace_character(cx_string *, cxsize, cxsize, cxchar, cxchar);

cx_string *cx_string_upper(cx_string *);
cx_string *cx_string_lower(cx_string *);
cx_string *cx_string_trim(cx_string *);
cx_string *cx_string_rtrim(cx_string *);
cx_string *cx_string_strip(cx_string *);

/*
 * Inserting and removing elements
 */

cx_string *cx_string_prepend(cx_string *, const cxchar *);
cx_string *cx_string_append(cx_string *, const cxchar *);
cx_string *cx_string_insert(cx_string *, cxssize, const cxchar *);
cx_string *cx_string_erase(cx_string *, cxssize, cxssize);
cx_string *cx_string_truncate(cx_string *, cxsize);

cx_string *cx_string_substr(const cx_string *, cxsize, cxsize);

/*
 *  Comparison functions
 */

cxbool cx_string_equal (const cx_string *, const cx_string *);
cxint cx_string_compare(const cx_string *, const cx_string *);
cxint cx_string_casecmp(const cx_string *, const cx_string *);
cxint cx_string_ncasecmp(const cx_string *, const cx_string *, cxsize);

/*
 * Search functions
 */

cxsize cx_string_find_first_not_of(const cx_string *, const cxchar *);
cxsize cx_string_find_last_not_of(const cx_string *, const cxchar *);

/*
 * I/O functions
 */

cxint cx_string_sprintf(cx_string *,
                        const cxchar *, ...) CX_GNUC_PRINTF(2, 3);
cxint cx_string_vsprintf(cx_string *,
                         const cxchar *, va_list) CX_GNUC_PRINTF(2, 0);

/*
 * Debugging utilities
 */

void cx_string_print(const cx_string *);

CX_END_DECLS

#endif /* CX_STRING_H */
