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


void rstr_free_func(char* dest) {
    rstr_free(dest);
}
int rstr_compare_func(const char* obj1, const char* obj2) {
    int ret = strcmp(obj1, obj2);
    if (ret == 0) {
        return rcode_eq;
    }
    else if (ret == 1)
    {
        return rcode_gt;
    }
    else {
        return rcode_lt;
    }
}

size_t rstr_cat(char* dest, const char* src, const size_t dest_size) {
    size_t position = rstr_len(dest);
    size_t src_len = rstr_len(src);
    size_t copy_len = dest_size == 0 ? src_len : rmin(src_len, dest_size - position);
    memcpy(dest + position, src, copy_len);
    position += copy_len;

    dest[position] = '\0';
    return copy_len;
}

char* rstr_concat(const char** src, const char* delim, bool suffix) {
	if (src == NULL || delim == NULL) {
		return rstr_empty;
	}

	char* str_cur = NULL;
	size_t delim_len = rstr_len(delim);
	size_t dest_len = 0;
	size_t cur_len = 0;
	size_t append_len = 0;

	rstr_array_for(src, str_cur) {
		dest_len += (rstr_len(str_cur) + delim_len);
	}

	char* dest = (char*)rstr_new(dest_len + 1u);

	rstr_array_for(src, str_cur) {
		cur_len = rstr_len(str_cur);
		memcpy(dest + append_len, str_cur, cur_len);
		append_len += cur_len;
		memcpy(dest + append_len, delim, delim_len);
		append_len += delim_len;
	}

	dest_len = suffix ? dest_len : dest_len - delim_len;
	dest[dest_len] = rstr_end;

	return dest;
}

char* rstr_fmt(char* dest, const char* fmt, const int max_len, ...) {
    if (!dest || !fmt) {
        return rstr_empty;
    }

    va_list ap;
    va_start(ap, max_len);
    int write_len = vsnprintf(dest, max_len, fmt, ap);
    va_end(ap);
    if (write_len < 0 || write_len >= max_len) {
        dest[0] = rstr_end;
        return rstr_empty;
    }
    return dest;
}

char* rstr_cpy(const void* data, size_t len){
    if (data == NULL) {
        return rstr_empty;
    }
	if (data == rstr_empty) {
		return rstr_empty;
	}
    unsigned int data_len = (unsigned int)strlen((char*)data);
    data_len = len > 0 && len < data_len ? len : data_len;
    char* data_copy = (char*)rstr_new(data_len + 1u);
    if (data_copy == NULL) {
        return rstr_empty;
    }
    //rstr_init(data_copy);
    memcpy(data_copy, data, data_len);
    data_copy[data_len] = rstr_end;
    return data_copy;
}

char* rstr_cpy_full(const void* data) {
    return rstr_cpy(data, 0);
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
    if (src == NULL || key == NULL) {
        //rassert(false, "invalid str.");
        return rstr_empty;
    }

    char* sub = strstr(src, key);
    if (sub == NULL) {
        return rstr_empty;
    }
    if (!new) {
        return sub;
    }

    return rstr_cpy(sub, 0);
}


int rstr_index(const char* src, const char* key) {
    //kmp_search(src, key, 0);
    char* start = NULL;
    
    start = strstr(src, key);
    if (start != NULL) {
        return start - src;
    }

    return -1;
}

int rstr_last_index(const char* src, const char* key) {
    int key_len = rstr_len(key);
    char* last = NULL;
    if (key_len == 1) {
        last = strrchr(src, key[0]);
        if (last != NULL) {
            return last - src;
        }
        return -1;
    }

    int index = -1;
    last = strstr(src, key);
    while (last != NULL) {
        index = last - src;
        last = strstr(last + sizeof(char), key);
    }

    return index;
}

/** 不支持unicode，有中文截断危险，utf8编码可以使用 **/
char* rstr_repl(char *src, char *dest_str, int dest_len, char *old_str, char *new_str) {
    if (!new_str || !dest_str) {
        return rstr_empty;
    }

    const size_t strLen = strlen(src);
    const size_t oldLen = strlen(old_str);
    const size_t newLen = strlen(old_str);
    //char bstr[strLen];//转换缓冲区
    //memset(bstr, 0, sizeof(bstr));

    dest_str[0] = rstr_end;

    for (size_t i = 0; i < strLen; i++) {
        if (!strncmp(src + i, old_str, oldLen)) {//查找目标字符串
            if (strlen(dest_str) + newLen >= dest_len) {
                dest_str[strlen(dest_str)] = rstr_end;
                break;
            }

            strcat(dest_str, new_str);
            i += oldLen - 1;
        }
        else {
            if (strlen(dest_str) + 1 >= dest_len) {
                dest_str[strlen(dest_str)] = rstr_end;
                break;
            }

            strncat(dest_str, src + i, 1);//保存一字节进缓冲区
        }
    }
    //strcpy(src, bstr);

    return dest_str;
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
    *token = rstr_array_end;

    rarray_free(array_tokens);

    return ret;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__