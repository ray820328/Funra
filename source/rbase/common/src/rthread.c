/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include <string.h>

#include "rthread.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__

long get_cur_thread_id() {
#if defined(_WIN32) || defined(_WIN64)
    return GetCurrentThreadId();
#elif __linux__
    return (long)syscall(SYS_gettid);
#elif defined(__APPLE__) && defined(__MACH__)
    return (long)syscall(SYS_thread_selfid);
#else
    return (long)pthread_self();
#endif /* defined(_WIN32) || defined(_WIN64) */
}

void rthread_init(rthread *t) {
    t->id = 0;
}

#if defined(_WIN32) || defined(_WIN64)
#pragma warning(disable : 4996)

#include <process.h>

static void rthread_errstr(rthread *t) {
    int ret_code;
    DWORD err = GetLastError();
    LPSTR errstr = 0;

    ret_code = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, err, 0, (LPSTR) &errstr, 0, NULL);
    if (ret_code != 0) {
        strncpy(t->err, errstr, sizeof(t->err) - 1);
        LocalFree(errstr);
    }
}

unsigned int __stdcall rthread_fn(void *arg) {
    rthread *t = arg;

    t->ret = t->rfunc(t->arg);
    return 0;
}

int rthread_start(rthread *t, void *(*rfunc)(void *), void *arg) {
    int ret_code = 0;

    t->rfunc = rfunc;
    t->arg = arg;

    t->id = (HANDLE) _beginthreadex(NULL, 0, rthread_fn, t, 0, NULL);
    if (t->id == 0) {
        rthread_errstr(t);
        ret_code = -1;
    }

    return ret_code;
}

int rthread_join(rthread *t, void **ret) {
    int ret_code = 0;
    void *val = NULL;
    DWORD rv;
    BOOL brc;

    if (t->id == 0) {
        rgoto(0);
    }

    rv = WaitForSingleObject(t->id, INFINITE);
    if (rv == WAIT_FAILED) {
        rthread_errstr(t);
        ret_code = -1;
    }

    brc = CloseHandle(t->id);
    if (!brc) {
        rthread_errstr(t);
        ret_code = -1;
    }

    val = t->ret;
    t->id = 0;

exit0:
    if (ret != NULL) {
        *ret = t->ret;
    }

    return ret_code;
}

#else // _WIN64

int rthread_start(rthread *t, void *(*rfunc)(void *), void *arg) {
    int ret_code;
    pthread_attr_t attr;

    ret_code = pthread_attr_init(&attr);
    if (ret_code != 0) {
        strncpy(t->err, strerror(ret_code), sizeof(t->err) - 1);
        return -1;
    }

    // This may only fail with EINVAL.
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    ret_code = pthread_create(&t->id, &attr, rfunc, arg);
    if (ret_code != 0) {
        strncpy(t->err, strerror(ret_code), sizeof(t->err) - 1);
    }

    // This may only fail with EINVAL.
    pthread_attr_destroy(&attr);

    return ret_code;
}

int rthread_join(rthread *t, void **ret) {
    int ret_code = 0;
    void *val = NULL;

    if (t->id == 0) {
        rgoto(0);
    }

    ret_code = pthread_join(t->id, &val);
    if (ret_code != 0) {
        strncpy(t->err, strerror(ret_code), sizeof(t->err) - 1);
    }

    t->id = 0;

exit0:
    if (ret != NULL) {
        *ret = val;
    }

    return ret_code;
}

#endif // _WIN64


int rthread_uninit(rthread *t) {
    int ret_code = rthread_join(t, NULL);
    // struct释放看留给上层，只释放thread系统资源
    return ret_code;
}

char *rthread_err(rthread *t) {
    return t->err;
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__