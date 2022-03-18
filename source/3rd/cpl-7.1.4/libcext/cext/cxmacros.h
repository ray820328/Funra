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


/*
 * This file MUST not include any other cext header file.
 */

#ifndef CX_MACROS_H
#define CX_MACROS_H


/*
 * Get the system's definition of NULL from stddef.h
 */

#include <stddef.h>


/*
 * An alias for __extension__
 */

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
#  define CX_GNUC_EXTENSION __extension__
#else
#  define CX_GNUC_EXTENSION
#endif


/*
 * Macros for the GNU compiler function attributes
 */

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#  define CX_GNUC_PURE   __attribute__((__pure__))
#  define CX_GNUC_MALLOC __attribute__((__malloc__))
#else
#  define G_GNUC_PURE
#  define G_GNUC_MALLOC
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#  define CX_GNUC_PRINTF(fmt_idx, arg_idx) \
          __attribute__((__format__ (__printf__, fmt_idx, arg_idx)))
#  define CX_GNUC_SCANF(fmt_idx, arg_idx)  \
          __attribute__((__format__ (__scanf__, fmt_idx, arg_idx)))
#  define CX_GNUC_FORMAT(arg_idx) __attribute__((__format_arg__ (arg_idx)))
#  define CX_GNUC_NORETURN        __attribute__((__noreturn__))
#  define CX_GNUC_CONST           __attribute__((__const__))
#  define CX_GNUC_UNUSED          __attribute__((__unused__))
#else
#  define CX_GNUC_PRINTF(fmt_idx, arg_idx)
#  define CX_GNUC_SCANF(fmt_idx, arg_idx)
#  define CX_GNUC_FORMAT(arg_idx)
#  define CX_GNUC_NORETURN
#  define CX_GNUC_CONST
#  define CX_GNUC_UNUSED
#endif


/*
 * Wrap the gcc __PRETTY_FUNCTION__ and __FUNCTION__ variables with macros.
 */

#if defined (__GNUC__) && (__GNUC__ < 3)
#  define CX_GNUC_FUNCTION         __FUNCTION__
#  define CX_GNUC_PRETTY_FUNCTION  __PRETTY_FUNCTION__
#else /* !__GNUC__ */
#  define CX_GNUC_FUNCTION         ""
#  define CX_GNUC_PRETTY_FUNCTION  ""
#endif /* !__GNUC__ */

#define CX_STRINGIFY(macro)         CX_STRINGIFY_ARG(macro)
#define CX_STRINGIFY_ARG(contents)  #contents


/*
 * String identifier for the current code position
 */

#if defined (__GNUC__) && (__GNUC__ < 3)
#  define CX_CODE_POS  __FILE__ ":" CX_STRINGIFY(__LINE__) ":" __PRETTY_FUNCTION__ "()"
#else
#  define CX_CODE_POS  __FILE__ ":" CX_STRINGIFY(__LINE__)
#endif


/*
 * Current function identifier
 */
#if defined (__GNUC__)
#  define CX_FUNC_NAME  ((const char*) (__PRETTY_FUNCTION__))
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 19901L
#  define CX_FUNC_NAME  ((const char*) (__func__))
#else
#  define CX_FUNC_NAME  ((const char*) ("???"))
#endif


/*
 * C code guard
 */

#undef CX_BEGIN_DECLS
#undef CX_END_DECLS

#ifdef __cplusplus
#  define CX_BEGIN_DECLS  extern "C" {
#  define CX_END_DECLS    }
#else
#  define CX_BEGIN_DECLS  /* empty */
#  define CX_END_DECLS    /* empty */
#endif


/*
 * Some popular macros. If the system provides already a definition for some
 * of them this definition is used, assuming the definition is correct.
 */

#ifndef NULL
#  ifdef __cplusplus
#    define NULL  (0L)
#  else /* !__cplusplus */
#    define NULL  ((void *) 0)
#  endif /* !__cplusplus */
#endif

#ifndef FALSE
#  define FALSE  (0)
#endif

#ifndef TRUE
#  define TRUE  (!FALSE)
#endif

#ifndef CX_MIN
# define CX_MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef CX_MAX
# define CX_MAX(a, b)  ((a) > (b) ? (a) : (b))
#endif

#ifndef CX_ABS
# define CX_ABS(a)  ((a) < (0) ? -(a) : (a))
#endif

#ifndef CX_CLAMP
# define CX_CLAMP(a, low, high)  (((a) > (high)) ? (high) : (((a) < (low)) ? (low) : (a)))
#endif


/*
 * Number of elements in an array
 */

#define CX_N_ELEMENTS(array)  (sizeof (array) / sizeof ((array)[0]))

#endif /* CX_MACROS_H */
