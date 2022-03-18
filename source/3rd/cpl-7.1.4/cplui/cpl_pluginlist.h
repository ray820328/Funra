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

#ifndef CPL_PLUGINLIST_H
#define CPL_PLUGINLIST_H

#include <stdio.h>

#include <cpl_macros.h>
#include <cpl_error.h>
#include <cpl_plugin.h>



CPL_BEGIN_DECLS

/**
 * @ingroup cpl_pluginlist
 *
 * @brief
 *   The opaque plugin list data type.
 */

typedef struct _cpl_pluginlist_ cpl_pluginlist;


/*
 * Create, copy and destroy operations.
 */

cpl_pluginlist *cpl_pluginlist_new(void) CPL_ATTR_ALLOC;
void cpl_pluginlist_delete(cpl_pluginlist *);


/*
 *  Accessor Functions
 */

cpl_error_code cpl_pluginlist_append(cpl_pluginlist *, const cpl_plugin *);
cpl_error_code cpl_pluginlist_prepend(cpl_pluginlist *, const cpl_plugin *);
cpl_plugin *cpl_pluginlist_get_first(cpl_pluginlist *);
cpl_plugin *cpl_pluginlist_get_last(cpl_pluginlist *);
cpl_plugin *cpl_pluginlist_get_next(cpl_pluginlist *);


/*
 *  Search Functions
 */

cpl_plugin *cpl_pluginlist_find(cpl_pluginlist *, const char *);

/*
 *  Size Functions
 */

int cpl_pluginlist_get_size(cpl_pluginlist *);

/*
 * Debugging
 */

void cpl_pluginlist_dump(const cpl_pluginlist *self, FILE *stream);

CPL_END_DECLS

#endif /* CPL_PLUGINLIST_H */
