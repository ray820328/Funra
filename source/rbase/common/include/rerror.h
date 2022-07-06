/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RERROR_H
#define RERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)

#define rerror_get_os_err() GetLastError()

#else //_WIN64
//linux
#include <errno.h>

#define rerror_get_os_err() -1

#endif //_WIN64

#ifdef __cplusplus
}
#endif

#endif //RERROR_H
