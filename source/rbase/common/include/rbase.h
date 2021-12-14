/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RBASE_H
#define RBASE_H

#ifdef __cplusplus
extern "C" {
#endif

#define raymalloc malloc
#define rayfree free

#define fn_raymalloc(x) raymalloc((x))
#define fn_rayfree(x) \
    do { \
        rayfree((x)); \
        (x) = NULL; \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif //RBASE_H
