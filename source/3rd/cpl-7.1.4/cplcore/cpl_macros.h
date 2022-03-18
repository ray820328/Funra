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

/*
 * This file must not include any other file than cxmacros.h
 */

#ifndef CPL_MACROS_H
#define CPL_MACROS_H

#include <cxmacros.h>

/*
 * C code guard
 */

#undef CPL_BEGIN_DECLS
#undef CPL_END_DECLS

#define CPL_BEGIN_DECLS  CX_BEGIN_DECLS
#define CPL_END_DECLS    CX_END_DECLS


/*
 * Macro to silence compiler warnings on unused arguments
 * and variables. Active by default, but can be turned off
 * for debug builds using CPL_WITH_UNUSED_WARNINGS.
 */

#ifdef CPL_UNUSED_WARNINGS
#  define CPL_UNUSED(arg)
#else
#  define CPL_UNUSED(arg) (void)(arg)
#endif


/* Needed to concatenate two and three macro arguments */
#define CPL_CONCAT(a,b) a ## _ ## b
#define CPL_CONCAT2X(a,b) CPL_CONCAT(a,b)
#define CPL_CONCAT3X(a,b,c) CPL_CONCAT2X(CPL_CONCAT2X(a,b),c)

#define CPL_XSTRINGIFY(TOSTRING) #TOSTRING
#define CPL_STRINGIFY(TOSTRING) CPL_XSTRINGIFY(TOSTRING)

/*

  (Try to) determine support for the function __attribute__

  These attributes are used in CPL (from the given gcc version)

   2.3  format (used only from gcc 3)
   2.5  const  (used only from gcc 3)
   2.96 pure   (used only from gcc 3)
   3.0  malloc
   3.1  deprecated
   3.3  nonnull
   3.4  warn_unused_result
   4.3  alloc_size

 */

#if defined __GNUC__ && __GNUC__ > 2
   /* gcc 3 or higher */

#  define CPL_ATTR_CONST       __attribute__((const))
#  define CPL_ATTR_PRINTF(A,B) __attribute__((format (printf, A, B)))
#  define CPL_ATTR_PURE        __attribute__((pure))

#  if __GNUC__ > 3 || defined __GNUC_MINOR__ && __GNUC_MINOR__ > 0
     /* gcc 3.1 or higher */
#    define CPL_ATTR_DEPRECATED __attribute__((deprecated))
#  endif

#  if __GNUC__ > 3 || defined __GNUC_MINOR__ && __GNUC_MINOR__ > 2
     /* gcc 3.3 or higher */
#    define CPL_ATTR_NONNULL __attribute__((nonnull))
#    define CPL_HAVE_ATTR_NONNULL
#    define CPL_UNLIKELY(x)     __builtin_expect((x), 0)
#    define CPL_LIKELY(x)       __builtin_expect((x), 1)
#    define CPL_ATTR_NOINLINE   __attribute__((noinline))
#  endif

#  if __GNUC__ > 3 || defined __GNUC_MINOR__ && __GNUC_MINOR__ > 3
     /* gcc 3.4 or higher */
#    define CPL_ATTR_ALLOC   __attribute__((malloc, warn_unused_result))

#    if __GNUC__ > 4 || __GNUC__ == 4 && defined __GNUC_MINOR__ && __GNUC_MINOR__ > 2
       /* gcc 4.3 or higher */

#      define CPL_ATTR_MALLOC                                           \
         __attribute__((malloc, warn_unused_result, alloc_size(1)))
#      define CPL_ATTR_CALLOC                                           \
         __attribute__((malloc, warn_unused_result, alloc_size(1,2)))
#      define CPL_ATTR_REALLOC                                          \
         __attribute__((warn_unused_result, alloc_size(2)))
#    else
       /* gcc 3.4 to 4.2 */
#      define CPL_ATTR_MALLOC  __attribute__((malloc, warn_unused_result))
#      define CPL_ATTR_CALLOC  __attribute__((malloc, warn_unused_result))
#      define CPL_ATTR_REALLOC __attribute__((warn_unused_result))
#    endif

#  else
     /* gcc 3.0 to 3.3 */

#    define CPL_ATTR_ALLOC   __attribute__((malloc))
#    define CPL_ATTR_MALLOC  __attribute__((malloc))
#    define CPL_ATTR_CALLOC  __attribute__((malloc))
#    define CPL_ATTR_REALLOC  /* __attribute__ */
#  endif

#  if __GNUC__ >= 4
#    define CPL_INTERNAL  __attribute__((visibility("hidden")))
#    define CPL_EXPORT    __attribute__((visibility("default")))
#  endif
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#define CPL_DIAG_PRAGMA_PUSH_IGN(x)                             \
    _Pragma("GCC diagnostic push")                              \
    _Pragma(CPL_STRINGIFY(GCC diagnostic ignored #x))
#define CPL_DIAG_PRAGMA_PUSH_ERR(x)                             \
    _Pragma("GCC diagnostic push")                              \
    _Pragma(CPL_STRINGIFY(GCC diagnostic error #x))
#define CPL_DIAG_PRAGMA_POP                     \
    _Pragma("GCC diagnostic pop")
#else
#define CPL_DIAG_PRAGMA_PUSH_IGN(x) /* pragma GCC diagnostic ignored */
#define CPL_DIAG_PRAGMA_PUSH_ERR(x) /* pragma GCC diagnostic error   */
#define CPL_DIAG_PRAGMA_POP         /* pragma GCC diagnostic pop     */
#endif


#ifndef CPL_ATTR_ALLOC
#  define CPL_ATTR_ALLOC /* __attribute__ */
#endif

#ifndef CPL_ATTR_CALLOC
#  define CPL_ATTR_CALLOC /*__attribute__ */
#endif

#ifndef CPL_ATTR_CONST
#  define CPL_ATTR_CONST /* __attribute__ */
#endif

#ifndef CPL_ATTR_DEPRECATED
#  define CPL_ATTR_DEPRECATED /* __attribute__ */
#endif

#ifndef CPL_ATTR_PURE
#  define CPL_ATTR_PURE /* __attribute__ */
#endif

#ifndef CPL_ATTR_MALLOC
#  define CPL_ATTR_MALLOC /* __attribute__ */
#endif

#ifndef CPL_ATTR_NONNULL
#  define CPL_ATTR_NONNULL /* __attribute__ */
#endif

#ifndef CPL_ATTR_PRINTF
#  define CPL_ATTR_PRINTF(A,B) /* __attribute__ */
#endif

#ifndef CPL_ATTR_REALLOC
#  define CPL_ATTR_REALLOC /* __attribute__ */
#endif

#ifndef CPL_UNLIKELY
#    define CPL_UNLIKELY(x) (x)
#    define CPL_LIKELY(x)   (x)
#endif

#ifndef CPL_ATTR_NOINLINE
#    define CPL_ATTR_NOINLINE /* __attribute__ */
#endif

#ifndef CPL_INTERNAL
#    define CPL_INTERNAL /* __attribute__ */
#    define CPL_EXPORT   /* __attribute__ */
#endif

#endif /* CPL_MACROS_H */
