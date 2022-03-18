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

#ifndef CPL_PARAMETER_H
#define CPL_PARAMETER_H

#include <stdio.h>

#include <cpl_error.h>
#include <cpl_type.h>


CPL_BEGIN_DECLS

/**
 * @ingroup cpl_parameter
 *
 * @brief
 *   Supported parameter modes.
 *
 * The parameter mode is used to set or get context specific parameter
 * attributes.
 */

enum _cpl_parameter_mode_ {

    /**
     * Command line mode of the parameter.
     * @hideinitializer
     */

    CPL_PARAMETER_MODE_CLI       = 1 << 0,

    /**
     * Environment variable mode of the parameter.
     * @hideinitializer
     */

    CPL_PARAMETER_MODE_ENV       = 1 << 1,

    /**
     * Configuration file mode of the parameter.
     * @hideinitializer
     */

    CPL_PARAMETER_MODE_CFG       = 1 << 2

};

/**
 * @ingroup cpl_parameter
 *
 * @brief
 *   The parameter mode data type.
 */

typedef enum _cpl_parameter_mode_ cpl_parameter_mode;


/**
 * @ingroup cpl_parameter
 *
 * @brief
 *   Supported parameter classes.
 */

enum _cpl_parameter_class_ {

    /**
     * Parameter of undefined or invalid class.
     * @hideinitializer
     */

    CPL_PARAMETER_CLASS_INVALID = 1 << 0,

    /**
     * Parameter representing a plain value.
     * @hideinitializer
     */

    CPL_PARAMETER_CLASS_VALUE   = 1 << 1,

    /**
     * Parameter representing a range of values.
     * @hideinitializer
     */

    CPL_PARAMETER_CLASS_RANGE   = 1 << 2,

    /**
     * Parameter representing an enumeration value.
     * @hideinitializer
     */

    CPL_PARAMETER_CLASS_ENUM    = 1 << 3

};


/**
 * @ingroup cpl_parameter
 *
 * @brief
 *   The parameter class data type.
 */

typedef enum _cpl_parameter_class_ cpl_parameter_class;


/**
 * @ingroup cpl_parameter
 *
 * @brief
 *   The opaque parameter data type.
 */

typedef struct _cpl_parameter_ cpl_parameter;


/*
 * Create and destroy operations
 */

cpl_parameter *cpl_parameter_new_value(const char *name, cpl_type type,
                                       const char *description,
                                       const char *context, ...) CPL_ATTR_ALLOC;
cpl_parameter *cpl_parameter_new_range(const char *name, cpl_type type,
                                       const char *description,
                                       const char *context, ...) CPL_ATTR_ALLOC;
cpl_parameter *cpl_parameter_new_enum(const char *name, cpl_type type,
                                      const char *description,
                                      const char *context, ...) CPL_ATTR_ALLOC;

cpl_parameter *cpl_parameter_duplicate(const cpl_parameter *other) CPL_ATTR_ALLOC;

void cpl_parameter_delete(cpl_parameter *self);


/*
 * Non-modifying operations
 */

cpl_type cpl_parameter_get_type(const cpl_parameter *self);
cpl_parameter_class cpl_parameter_get_class(const cpl_parameter *self);


int cpl_parameter_is_enabled(const cpl_parameter *self,
                             cpl_parameter_mode mode);

/*
 * Assignment operations
 */

cpl_error_code cpl_parameter_set_bool(cpl_parameter *self, int value);
cpl_error_code cpl_parameter_set_int(cpl_parameter *self, int value);
cpl_error_code cpl_parameter_set_double(cpl_parameter *self, double value);
cpl_error_code cpl_parameter_set_string(cpl_parameter *self,
                                        const char *value);

cpl_error_code cpl_parameter_set_default_bool(cpl_parameter *self,
                                              int value);
cpl_error_code cpl_parameter_set_default_int(cpl_parameter *self,
                                             int value);
cpl_error_code cpl_parameter_set_default_double(cpl_parameter *self,
                                                double value);
cpl_error_code cpl_parameter_set_default_string(cpl_parameter *self,
                                                const char *value);

cpl_error_code cpl_parameter_set_default_flag(cpl_parameter *self,
                                              int status);

/*
 * Element access
 */

const char *cpl_parameter_get_name(const cpl_parameter *self);
const char *cpl_parameter_get_context(const cpl_parameter *self);
const char *cpl_parameter_get_help(const cpl_parameter *self);

int cpl_parameter_get_default_flag(const cpl_parameter *self);

int cpl_parameter_get_range_min_int(const cpl_parameter *self);
double cpl_parameter_get_range_min_double(const cpl_parameter *self);
int  cpl_parameter_get_range_max_int(const cpl_parameter *self);
double cpl_parameter_get_range_max_double(const cpl_parameter *self);

int cpl_parameter_get_enum_size(const cpl_parameter *self);
int cpl_parameter_get_enum_int(const cpl_parameter *self, int position);
double cpl_parameter_get_enum_double(const cpl_parameter *self, int position);
const char *cpl_parameter_get_enum_string(const cpl_parameter *self,
                                          int position);

int cpl_parameter_get_default_bool(const cpl_parameter *self);
int cpl_parameter_get_default_int(const cpl_parameter *self);
double cpl_parameter_get_default_double(const cpl_parameter *self);
const char *cpl_parameter_get_default_string(const cpl_parameter *self);

int cpl_parameter_get_bool(const cpl_parameter *self);
int cpl_parameter_get_int(const cpl_parameter *self);
double cpl_parameter_get_double(const cpl_parameter *self);
const char *cpl_parameter_get_string(const cpl_parameter *self);

/*
 * Miscellaneous
 */

cpl_error_code cpl_parameter_enable(cpl_parameter *self,
                                    cpl_parameter_mode mode);
cpl_error_code cpl_parameter_disable(cpl_parameter *self,
                                     cpl_parameter_mode mode);

int cpl_parameter_get_id(const cpl_parameter *self);
cpl_error_code cpl_parameter_set_id(cpl_parameter *self, int id);

const char *cpl_parameter_get_tag(const cpl_parameter *self);

cpl_error_code cpl_parameter_set_tag(cpl_parameter *self, const char *tag);

const char *cpl_parameter_get_alias(const cpl_parameter *self,
                                    cpl_parameter_mode mode);

cpl_error_code cpl_parameter_set_alias(cpl_parameter *self,
                                       cpl_parameter_mode mode,
                                       const char *alias);

/*
 * Debugging
 */

void cpl_parameter_dump(const cpl_parameter *self, FILE *stream);

CPL_END_DECLS

#endif /* CPL_PARAMETER_H */
