/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RTHREAD_H
#define RTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcommon.h"

/* ------------------------------- Macros ------------------------------------*/

typedef void *(*rthread_func)(void *);

#if defined(_WIN32) || defined(_WIN64)

#define rmutex_t CRITICAL_SECTION
#define rmutex_init(rmutexObj) \
    InitializeCriticalSection(rmutexObj)
#define rmutex_uninit(rmutexObj) \
	DeleteCriticalSection(rmutexObj)

#define rmutex_try_lock(rmutexObj) \
    if (TryEnterCriticalSection(mutex))
#define rmutex_lock(rmutexObj) \
    EnterCriticalSection(rmutexObj)
#define rmutex_unlock(rmutexObj) \
    LeaveCriticalSection(rmutexObj)

#else /* defined(_WIN32) || defined(_WIN64) */

#define rmutex_t pthread_mutex_t
#define rmutex_init(rmutexObj) \
    pthread_mutex_init(rmutexObj, NULL)
#define rmutex_uninit(rmutexObj) \
	pthread_mutex_destroy(rmutexObj);

//pthread_mutexattr_t attr;
//pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)
#define rmutex_try_lock(rmutexObj) \
{ \
	int err; \
	err = pthread_mutex_trylock(rmutexObj); \
	if (err) { \
		if (err != EBUSY && err != EAGAIN) { \
			rassert(false, ""); \
		} else { \
			rdebug("locked."); \
		} \
	} else 
#define rmutex_lock(rmutexObj) \
    pthread_mutex_lock(rmutexObj)
#define rmutex_unlock(rmutexObj) \
    pthread_mutex_unlock(rmutexObj) 

#endif /* defined(_WIN32) || defined(_WIN64) */

/* ------------------------------- APIs ------------------------------------*/

long rthread_cur_id();

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

typedef struct rthread_s {
    HANDLE id;
    rthread_func rfunc;
    void *arg;
    void *ret;
    char err[128];
} rthread_t;

#else // _WIN64

#include <pthread.h>

typedef struct rthread_s {
    pthread_t id;
    char err[128];
} rthread_t;

#endif // _WIN64

void rthread_init(rthread_t *t);

/**
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error, call 'sc_thread_err()' for error string.
 */
int rthread_uninit(rthread_t *t);

/**
 * @param t thread
 * @return  last error message
 */
char* rthread_err(rthread_t *t);

/**
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error
 */
int rthread_start(rthread_t *t, rthread_func rfunc, void *arg);

/**
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error
 */
int rthread_join(rthread_t *t, void **ret);

/**
 * 终止调用者本身 (尽量不要调)
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error
 */
int rthread_exit(rthread_t *t, void **ret);

/**
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error
 */
int rthread_detach(rthread_t *t, void **ret);

#ifdef __cplusplus
}
#endif

#endif //RTHREAD_H
