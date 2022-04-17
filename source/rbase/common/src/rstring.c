/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rstring.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__

size_t rstr_cat(char* dest, const char* src, const size_t sizeofDest) {
    size_t position = strlen(dest);
    size_t srcLen = strlen(src);
    size_t copyLen = rmin(srcLen, sizeofDest - strlen(dest) - 1);
    memcpy(dest + position, src, copyLen);
    position += copyLen;

    dest[position] = '\0';
    return copyLen - srcLen;
}
char* rstr_fmt(char* dest, const char* fmt, const int maxLen, ...) {
    if (!dest || !fmt) {
        return NULL;
    }
    //char* dest = raymalloc((int64_t)(maxLen + 1u));
    //if (!dest) {
    //    return NULL;
    //}
    va_list ap;
    va_start(ap, maxLen);
    int writeLen = vsnprintf(dest, maxLen, fmt, ap);
    va_end(ap);
    if (writeLen < 0) {
        //    rayfree((void*)dest);
        dest[0] = rstr_end;
        return NULL;
    }
    return dest;
}
char* rstr_cpy(const void *key) {//, int maxLen){
    if (!key) {
        return NULL;
    }
    unsigned int keyLen = (unsigned int)strlen((char*)key);
    char* keyCopy = raymalloc((int64_t)(keyLen + 1u));
    if (!keyCopy) {
        return NULL;
    }
    memcpy(keyCopy, key, keyLen);
    keyCopy[keyLen] = rstr_end;
    return keyCopy;
}

/** 有中文截断危险 **/
char* rstr_repl(char *src, char *destStr, int destLen, char *oldStr, char *newStr) {
    if (!newStr || !destStr) {
        return src;
    }

    const size_t strLen = strlen(src);
    const size_t oldLen = strlen(oldStr);
    const size_t newLen = strlen(oldStr);
    //char bstr[strLen];//转换缓冲区
    //memset(bstr, 0, sizeof(bstr));

    destStr[0] = rstr_end;

    for (size_t i = 0; i < strLen; i++) {
        if (!strncmp(src + i, oldStr, oldLen)) {//查找目标字符串
            if (strlen(destStr) + newLen >= destLen) {
                destStr[strlen(destStr)] = rstr_end;
                break;
            }

            strcat(destStr, newStr);
            i += oldLen - 1;
        }
        else {
            if (strlen(destStr) + 1 >= destLen) {
                destStr[strlen(destStr)] = rstr_end;
                break;
            }

            strncat(destStr, src + i, 1);//保存一字节进缓冲区
        }
    }
    //strcpy(src, bstr);

    return destStr;
}
R_API inline void rstr_free(const void *key) {
    if (key)
        rayfree((void*)key);
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__