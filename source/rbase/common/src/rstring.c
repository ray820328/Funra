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
    size_t position = rstr_len(dest);
    size_t srcLen = rstr_len(src);
    size_t copyLen = rmin(srcLen, sizeofDest - position - 1);
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
    char* keyCopy = (char*)rstr_new(keyLen + 1u);
    rstr_init(keyCopy);
    if (!keyCopy) {
        return NULL;
    }
    memcpy(keyCopy, key, keyLen);
    keyCopy[keyLen] = rstr_end;
    return keyCopy;
}

char* rstr_substr(const char *src, const size_t dest_size)
{
    size_t length=0;
    char *p;
    const char *q;

    char *dst = raymalloc((int64_t)(dest_size));

    assert(dst != NULL);
    assert(src != (const char *) NULL);
    assert(dest_size >= 1);

    for ( p=dst, q=src, length=0; (*q != 0) && (length < dest_size-1); length++, p++, q++ ){
        *p = *q;
    }

    dst[length]='\0';

    return dst;
}

/** 不支持unicode，有中文截断危险，utf8编码可以使用 **/
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

R_API int rstr_token(const char *src, const char *delim, char** tokens) {

    return rcode_ok;
}
//
//char* rstr_token(char* str, const char* sep, char** last)
//{
//    char* token = NULL;
//
//    if (!str)           /* subsequent call */
//        str = *last;    /* start where we left off */
//
//    /* skip characters in sep (will terminate at '\0') */
//    while (*str && strchr(sep, *str))
//        ++str;
//
//    if (!*str)          /* no more tokens */
//        return NULL;
//
//    token = str;
//
//    /* skip valid token characters to terminate token and
//     * prepare for the next call (will terminate at '\0') 
//     */
//    *last = token + 1;
//    while (**last && !strchr(sep, **last))
//        ++*last;
//
//    if (**last) {
//        **last = '\0';
//        ++*last;
//    }
//
//    return token;
//}
//
//int apr_filepath_list_split_impl(apr_array_header_t **pathelts, const char *liststr, char separator, apr_pool_t *p)
//{
//    char *path, *part, *ptr;
//    char separator_string[2] = { '\0', '\0' };
//    apr_array_header_t *elts;
//    int nelts;
//
//    separator_string[0] = separator;
//    /* Count the number of path elements. We know there'll be at least
//       one even if path is an empty string. */
//    path = apr_pstrdup(p, liststr);
//    for (nelts = 0, ptr = path; ptr != NULL; ++nelts)
//    {
//        ptr = strchr(ptr, separator);
//        if (ptr)
//            ++ptr;
//    }
//
//    /* Split the path into the array. */
//    elts = apr_array_make(p, nelts, sizeof(char*));
//    while ((part = rstr_token(path, separator_string, &ptr)) != NULL)
//    {
//        if (*part == '\0')      /* Ignore empty path components. */
//            continue;
//
//        *(char**)apr_array_push(elts) = part;
//        path = NULL;            /* For the next call to rstr_token */
//    }
//
//    *pathelts = elts;
//
//    return rcode_ok;
//}
//
//
//apr_status_t apr_filepath_list_merge_impl(char **liststr,
//                                          apr_array_header_t *pathelts,
//                                          char separator,
//                                          apr_pool_t *p)
//{
//    apr_size_t path_size = 0;
//    char *path;
//    int i;
//
//    /* This test isn't 100% certain, but it'll catch at least some
//       invalid uses... */
//    if (pathelts->elt_size != sizeof(char*))
//        return APR_EINVAL;
//
//    /* Calculate the size of the merged path */
//    for (i = 0; i < pathelts->nelts; ++i)
//        path_size += strlen(((char**)pathelts->elts)[i]);
//
//    if (path_size == 0)
//    {
//        *liststr = NULL;
//        return APR_SUCCESS;
//    }
//
//    if (i > 0)                  /* Add space for the separators */
//        path_size += (i - 1);
//
//    /* Merge the path components */
//    path = *liststr = apr_palloc(p, path_size + 1);
//    for (i = 0; i < pathelts->nelts; ++i)
//    {
//        /* ### Hmmmm. Calling strlen twice on the same string. Yuck.
//               But is is better than reallocation in apr_pstrcat? */
//        const char *part = ((char**)pathelts->elts)[i];
//        apr_size_t part_size = strlen(part);
//        if (part_size == 0)     /* Ignore empty path components. */
//            continue;
//
//        if (i > 0)
//            *path++ = separator;
//        memcpy(path, part, part_size);
//        path += part_size;
//    }
//    *path = '\0';
//    return APR_SUCCESS;
//}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__