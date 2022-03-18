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

#ifndef CXTHREAD_H_
#define CXTHREAD_H_

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_PTHREAD_H
#  include <pthread.h>
#endif

#include <cxtypes.h>


/*
 * Map local types and functions to the POSIX thread model implementation
 */

#if defined(CX_THREADS_ENABLED)

#if defined(HAVE_PTHREAD_H)

#define CX_STATIC_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
#define CX_STATIC_ONCE_INIT  PTHREAD_ONCE_INIT

#define CX_MUTEX_TYPE_DEFAULT    PTHREAD_MUTEX_DEFAULT
#define CX_MUTEX_TYPE_NORMAL     PTHREAD_MUTEX_NORMAL
#define CX_MUTEX_TYPE_RECURSIVE  PTHREAD_MUTEX_RECURSIVE

typedef pthread_mutex_t cx_mutex;
typedef pthread_once_t  cx_once;
typedef pthread_key_t   cx_private;

#define cx_mutex_init(mutex, type)                 \
    do {                                           \
        pthread_mutexattr_t attr;                  \
                                                   \
        pthread_mutexattr_init(&attr);             \
        pthread_mutexattr_settype(&attr, (type));  \
                                                   \
        pthread_mutex_init(mutex, &attr);          \
                                                   \
        pthread_mutexattr_destroy(&attr);          \
    }                                              \
    while (0)

#define cx_mutex_lock(mutex)    pthread_mutex_lock((mutex))
#define cx_mutex_trylock(mutex) pthread_mutex_trylock((mutex))
#define cx_mutex_unlock(mutex)  pthread_mutex_unlock((mutex))

#define cx_thread_once(name, func, args)  pthread_once(&(name), (func))

#define cx_private_init(name, func)  pthread_key_create(&(name), (func))
#define cx_private_set(name, data)   pthread_setspecific((name), (data))
#define cx_private_get(name)         pthread_getspecific((name))

#else  /* !HAVE_PTHREAD_H */
#  error "Thread support is requested, but POSIX thread model is not present!"
#endif /* !HAVE_PTHREAD_H */

#else  /* !CX_THREADS_ENABLED */

typedef struct cx_private cx_private;

#define cx_mutex_init(mutex, type) /* empty */

#define cx_mutex_lock(mutex)       /* empty */
#define cx_mutex_trylock(mutex)    /* empty */
#define cx_mutex_unlock(mutex)     /* empty */

#define cx_thread_once(name, func, args) (func)()

#define cx_private_init(name, func)  /* empty */
#define cx_private_set(name, data)   ((name) = (data))
#define cx_private_get(name)         (name)

#endif /* !CX_THREADS_ENABLED */


/*
 * Convenience macros to setup locks for global variables.
 * These macros expand to nothing, if thread support was not enabled.
 */

#define CX_LOCK_NAME(name)  _cx__ ## name ## _lock

#if defined(CX_THREADS_ENABLED)

#  define CX_LOCK_DEFINE_STATIC(name)  static CX_LOCK_DEFINE(name)
#  define CX_LOCK_DEFINE(name)         cx_mutex CX_LOCK_NAME(name)
#  define CX_LOCK_EXTERN(name)         extern cx_mutex CX_LOCK_NAME(name)

#  define CX_LOCK_DEFINE_INITIALIZED_STATIC(name)  \
          static CX_LOCK_DEFINE_INITIALIZED(name)
#  define CX_LOCK_DEFINE_INITIALIZED(name)         \
          CX_LOCK_DEFINE(name) = CX_STATIC_MUTEX_INIT

#  define CX_INITLOCK(name, type)  cx_mutex_init(&CX_LOCK_NAME(name), (type))

#  define CX_LOCK(name)     cx_mutex_lock(&CX_LOCK_NAME(name))
#  define CX_TRYLOCK(name)  cx_mutex_trylock(&CX_LOCK_NAME(name))
#  define CX_UNLOCK(name)   cx_mutex_unlock(&CX_LOCK_NAME(name))

#else /* !CX_THREADS_ENABLED */

#  define CX_LOCK_DEFINE_STATIC(name)  /* empty */
#  define CX_LOCK_DEFINE(name)         /* empty */
#  define CX_LOCK_EXTERN(name)         /* empty */

#  define CX_LOCK_DEFINE_INITIALIZED_STATIC(name)  /* empty */
#  define CX_LOCK_DEFINE_INITIALIZED(name)         /* empty */

#  define CX_INITLOCK(name, type)  /* empty */

#  define CX_LOCK(name)     /* empty */
#  define CX_TRYLOCK(name)  (TRUE)
#  define CX_UNLOCK(name)   /* empty */

#endif /* !CX_THREADS_ENABLED */


/*
 * Convenience macros for setting up mutexes for one time initalizations
 */

#if defined(CX_THREADS_ENABLED)

#  define CX_ONCE_DEFINE_STATIC(name)  static CX_ONCE_DEFINE(name)
#  define CX_ONCE_DEFINE(name)         cx_once (name)

#  define CX_ONCE_DEFINE_INITIALIZED_STATIC(name)  \
          static CX_ONCE_DEFINE_INITIALIZED(name)
#  define CX_ONCE_DEFINE_INITIALIZED(name)         \
          cx_once (name) = CX_STATIC_ONCE_INIT

#else /* !CX_THREADS_ENABLED */

#  define CX_ONCE_DEFINE_STATIC(name)  /* empty */
#  define CX_ONCE_DEFINE(name)         /* empty */

#  define CX_ONCE_DEFINE_INITIALIZED_STATIC(name)  /* empty */
#  define CX_ONCE_DEFINE_INITIALIZED(name)         /* empty */

#endif /* !CX_THREADS_ENABLED */


/*
 * Convenience macros for setting up thread-specific data
 */

#if defined(CX_THREADS_ENABLED)

#  define CX_PRIVATE_DEFINE_STATIC(name)  cx_private (name)

#else /* !CX_THREADS_ENABLED */

#  define CX_PRIVATE_DEFINE_STATIC(name)  static cx_private *(name)

#endif /* !CX_THREADS_ENABLED */

#endif /* CXTHREAD_H_ */
