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

#if defined(_WIN32) || defined(_WIN64)

#define rmutex_t CRITICAL_SECTION
#define rmutex_init(rmutexObj) \
    InitializeCriticalSection(rmutexObj)
#define rmutex_uninit(rmutexObj) \
    
#define rmutex_lock(rmutexObj) \
    EnterCriticalSection(rmutexObj)
#define rmutex_unlock(rmutexObj) \
    LeaveCriticalSection(rmutexObj)

#else /* defined(_WIN32) || defined(_WIN64) */

#define rmutex_t pthread_mutex_t
#define rmutex_init(rmutexObj) \
    pthread_mutex_init(rmutexObj, NULL)
#define rmutex_uninit(rmutexObj) \
    
#define rmutex_lock(rmutexObj) \
    pthread_mutex_lock(rmutexObj)
#define rmutex_unlock(rmutexObj) \
    pthread_mutex_unlock(rmutexObj) 

#endif /* defined(_WIN32) || defined(_WIN64) */

/* ------------------------------- APIs ------------------------------------*/

long get_cur_thread_id();

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

typedef struct rthread {
    HANDLE id;
    void *(*rfunc)(void *);
    void *arg;
    void *ret;
    char err[128];
} rthread;

#else // _WIN64

#include <pthread.h>

typedef struct rthread {
    pthread_t id;
    char err[128];
} rthread;

#endif // _WIN64

void rthread_init(rthread *t);

/**
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error, call 'sc_thread_err()' for error string.
 */
int rthread_uninit(rthread *t);

/**
 * @param t thread
 * @return  last error message
 */
char* rthread_err(rthread *t);

/**
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error, call 'sc_thread_err()' for error string.
 */
int rthread_start(rthread *t, void *(*rfunc)(void *), void *arg);

/**
 * @param t thread
 * @return  '0' on success,
 *          '-1' on error, call 'sc_thread_err()' for error string.
 */
int rthread_join(rthread *t, void **ret);


#ifdef __cplusplus
}
#endif

#endif //RTHREAD_H
