/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#if defined(_WIN32) || defined(_WIN64)

#else

#include <regex.h>

#endif

#include "rstring.h"
#include "rarray.h"
#include "rlog.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__

char* rstr_join(const char* src, ...) {
    va_list argp;
    char* temp = NULL;
    int temp_len = 0;
    int max_len = 256;
    int expand_count = 1;
    char buf[256];
    char* buf_ext = buf;
    char* buf_temp = NULL;
    int pos = 0;

    va_start(argp, src);
    temp = src;

    while (temp != rstr_array_end) {
        temp_len = rstr_len(temp);
        pos += temp_len;

        if (pos < max_len * expand_count) {
            memcpy(buf_ext + pos - temp_len, temp, temp_len);
        }
        else {
            do {
                expand_count *= 2;
            } while (max_len * expand_count < pos);

            buf_temp = rstr_new(max_len * expand_count);
            memcpy(buf_temp, buf_ext, pos - temp_len);
            memcpy(buf_temp + pos - temp_len, temp, temp_len);

            if (buf_ext != buf) {
                rstr_free(buf_ext);
            }

            buf_ext = buf_temp;
            buf_temp = NULL;
        }

        temp = va_arg(argp, char*);
    }
    va_end(argp);

    buf_ext[pos] = rstr_end;

    if (expand_count == 1) {
        return rstr_cpy(buf, 0);
    }

    return buf_ext;
}

char** rstr_make_array(const int count, ...) {
    va_list argp;
    char** rstr_arr = rnew_data_array(sizeof(char*), count + 1);
    rstr_arr[count] = rstr_array_end;
    char* temp = NULL;
    int index = 0;

    va_start(argp, count);
    while (index < count) {
        temp = va_arg(argp, char*);
        if (temp == rstr_array_end) {
            temp = rstr_empty;
        }
        rstr_arr[index] = rstr_cpy(temp, 0);
        ++index;
    }
    va_end(argp);

    return rstr_arr;
}

/** src: "ray820328@163.com" pat = "h{3,9}(.*)@.{3}.(.*)" */
static int _regex(char* src, char* pat, rarray_t* array_ins) {
    //char errbuf[1024];
    //regex_t reg;
   
    //int err,num = 9;
    //regmatch_t matchs[9];

    //if (regcomp(&reg, pat, REG_EXTENDED) < 0) {
    //    regerror(err, &reg, errbuf, sizeof(errbuf));
    //    rwarn("err:%s\n", errbuf);
    //    return -1;
    //}

    //err = regexec(&reg, src, num, matchs, 0);

    //if (err == REG_NOMATCH) {
    //    rinfo("no match\n");
    //    return rcode_ok;
    //}
    //else if (err) {
    //    regerror(err, &reg, errbuf, sizeof(errbuf));
    //    rwarn("err:%s\n", errbuf);
    //    return -1;
    //}

    //for (int i = 0; i < 10 && matchs[i].rm_so != -1; i++) {
    //    int len = matchs[i].rm_eo - matchs[i].rm_so;
    //    if (len) {
    //        char* record = rstr_new(len + 1);
    //        memcpy(record, src + matchs[i].rm_so, len);
    //        record[len] = rstr_end;
    //        rarray_add(array_ins, record);
    //    }
    //}

    return rcode_ok;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__