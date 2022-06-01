/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rstring.h"
#include "rarray.h"
#include "rlog.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__

static void _kmp_next_array_init(char* pat, int len_pattern, rarray_t* array_ins) {
    int length = 0;
    rarray_add(array_ins, 0);
    int i = 1;
    while (i < len_pattern) {
        if (pat[i] == pat[length]) {
            length++;
            rarray_add(array_ins, length);
            i++;
        }
        else {
            if (length != 0) {
                rarray_add(array_ins, length - 1);
            }
            else {
                rarray_add(array_ins, 0);
                i++;
            }
        }
    }
}
static rarray_t* _kmp_search(char* str, char* pattern, int count) {
    const int len_pattern = strlen(pattern);
    int len_src = strlen(str);

    static rarray_t* array_key = NULL;
    if (array_key == NULL) {
        rarray_init(array_key, rdata_type_int, 30);
    }
    rarray_clear(array_key);

    _kmp_next_array_init(pattern, len_pattern, array_key);

    rarray_t* array_ret = NULL;
    if (array_ret == NULL) {
        rarray_init(array_ret, rdata_type_int, 10);
    }

    int i = 0;
    int j = 0;
    while (i < len_src) {
        if (pattern[j] == str[i]) {
            j++;
            i++;
        }
        if (j == len_pattern) {
            rarray_add(array_ret, i - j);
            if (count > 0) {
                if (--count == 0) {
                    return array_ret;
                }
            }

            j = rarray_at(array_key, j - 1);
        }
        else if (i < len_src && pattern[j] != str[i]) {
            if (j != 0) {
                j = rarray_at(array_key, j - 1);
            }
            else {
                i = i + 1;
            }
        }
    }

    return array_ret;
}


size_t rstr_cat(char* dest, const char* src, const size_t dest_size) {
    size_t position = rstr_len(dest);
    size_t src_len = rstr_len(src);
    size_t copy_len = rmin(src_len, dest_size - position - 1);
    memcpy(dest + position, src, copy_len);
    position += copy_len;

    dest[position] = '\0';
    return copy_len - src_len;
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

char* rstr_cpy(const void* data, size_t len){
    if (data == NULL) {
        return NULL;
    }
    unsigned int data_len = (unsigned int)strlen((char*)data);
    data_len = len > 0 && len < data_len ? len : data_len;
    char* data_copy = (char*)rstr_new(data_len + 1u);
    rstr_init(data_copy);
    if (data_copy == NULL) {
        return NULL;
    }
    memcpy(data_copy, data, data_len);
    data_copy[data_len] = rstr_end;
    return data_copy;
}

char* rstr_sub(const char* src, const size_t from, const size_t dest_size, bool new) {
    if (!src || from  < 0 || dest_size < 0 || rstr_len(src) < (from + dest_size)) {
        rassert(false, "invalid str.");
        return rstr_empty;
    }

    if (!new) {
        return src + from;//整个from字符串，不仅仅dest
    }

    char* dest = rstr_new(dest_size + 1u);
    rassert(dest != NULL, "dest is null.");

    memcpy(dest, src + from, dest_size);
    //size_t length=0;
    //char *p;
    //const char *q;
    //for ( p=dst, q=src, length=0; (*q != 0) && (length < from + dest_size - 1); length++, p++, q++ ){
    //    *p = *q;
    //}

    dest[dest_size] = '\0';

    return dest;
}

char* rstr_sub_str(const char* src, const char* key, bool new) {
    if (!src || !key) {
        rassert(false, "invalid str.");
        return rstr_empty;
    }

    char* sub = strstr(src, key);
    if (sub != NULL) {
        return rstr_empty;
    }
    if (!new) {
        return sub;
    }

    size_t sub_len = rstr_len(sub);
    char* dest = rstr_new(sub_len + 1u);

    memcpy(dest, sub, sub_len);

    dest[sub_len] = '\0';

    return dest;
}


size_t rstr_index(const char* src, const char* key) {
    //kmp_search(src, key, 0);
    return 0;
}

size_t rstr_last_index(const char* src, const char* key) {

    return 0;
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

char** rstr_split(const char* src, const char* delim) {
    //todo Ray 根据长度走短字符串直接走遍历

    //char* token = strtok(src, delim);
    //while (token != NULL) {
    //    rarray_add(array_ins, token);
    //    token = strtok(NULL, delim);
    //}
    //return rarray_get_all(array_ins);

    rarray_t* array_tokens = _kmp_search(src, delim, 0);
    rarray_size_t data_len = rarray_size(array_tokens);
    if (data_len == 0) {
        rarray_free(array_tokens);
        return NULL;
    }

    //rarray_t* array_ins = NULL;
    //rarray_init(array_ins, rdata_type_string, data_len + 1);

    char** ret = rstr_array_new(data_len + 1);
    char** token = NULL;
    size_t src_len = rstr_len(src);
    size_t delim_len = rstr_len(delim);
    size_t from_pos = 0;
    size_t sub_len = rarray_at(array_tokens, 0);

    //rarray_add(array_ins, rstr_sub(src, from_pos, sub_len, true));
    token = ret;
    *token = rstr_sub(src, from_pos, sub_len, true);

    for (int index = 1; index < data_len; index++) {
        from_pos = (rdata_type_int_inner_type)rarray_at(array_tokens, index - 1) + delim_len;
        sub_len = (rdata_type_int_inner_type)rarray_at(array_tokens, index) - from_pos;
        token = ret + index;
        *token = rstr_sub(src, from_pos, sub_len, true);
    }

    from_pos = (rdata_type_int_inner_type)rarray_at(array_tokens, data_len - 1) + delim_len;
    sub_len = src_len - from_pos;
    token = ret + data_len;
    *token = rstr_sub(src, from_pos, sub_len, true);

    token = ret + data_len + 1;
    *token = NULL;

    rarray_free(array_tokens);

    return ret;
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