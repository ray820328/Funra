/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RMEMORY_H
#define RMEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#define raymalloc(x) malloc((x))
#define raycmalloc(x, elem_type) (elem_type*) calloc((x), sizeof(elem_type))
#define rayfree(x) \
    do { \
        free((x)); \
        (x) = NULL; \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif //RMEMORY_H
