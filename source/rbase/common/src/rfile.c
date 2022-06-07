/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "rfile.h"
#include "rlog.h"
#include "rstring.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif //__GNUC__

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>

static int unlink(const char *pathname)
{
    return DeleteFile(pathname) ? 0 : -1; /* 0:success, -1:failure */
}

static int convert_utf8_to_ucs2(const char *in, size_t *inbytes, WCHAR *out, size_t *outwords)
{
    int64_t newch, mask;
    size_t expect, eating;
    int ch;

    while (*inbytes && *outwords)
    {
        ch = (unsigned char)(*in++);
        if (!(ch & 0200)) {
            /* US-ASCII-7 plain text */
            --*inbytes;
            --*outwords;
            *(out++) = ch;
        }
        else
        {
            if ((ch & 0300) != 0300) {
                /* Multibyte Continuation is out of place */
                return -1;
            }
            else
            {
                /* Multibyte Sequence Lead Character
                 * Compute the expected bytes while adjusting
                 * or lead byte and leading zeros mask.
                 */
                mask = 0340;
                expect = 1;
                while ((ch & mask) == mask) {
                    mask |= mask >> 1;
                    if (++expect > 3) /* (truly 5 for ucs-4) */
                        return -1;
                }
                newch = ch & ~mask;
                eating = expect + 1;
                if (*inbytes <= expect)
                    return -2;
                /* Reject values of excessive leading 0 bits
                 * utf-8 _demands_ the shortest possible byte length
                 */
                if (expect == 1) {
                    if (!(newch & 0036))
                        return -1;
                }
                else {
                    /* Reject values of excessive leading 0 bits */
                    if (!newch && !((unsigned char)*in & 0077 & (mask << 1)))
                        return -1;
                    if (expect == 2) {
                        /* Reject values D800-DFFF when not utf16 encoded
                         * (may not be an appropriate restriction for ucs-4)
                         */
                        if (newch == 0015 && ((unsigned char)*in & 0040))
                            return -1;
                    }
                    else if (expect == 3) {
                        /* Short circuit values > 110000 */
                        if (newch > 4)
                            return -1;
                        if (newch == 4 && ((unsigned char)*in & 0060))
                            return -1;
                    }
                }
                /* Where the boolean (expect > 2) is true, we will need
                 * an extra word for the output.
                 */
                if (*outwords < (size_t)(expect > 2) + 1)
                    break; /* buffer full */
                while (expect--)
                {
                    /* Multibyte Continuation must be legal */
                    if (((ch = (unsigned char)*(in++)) & 0300) != 0200)
                        return -1;
                    newch <<= 6;
                    newch |= (ch & 0077);
                }
                *inbytes -= eating;
                /* newch is now a true ucs-4 character
                 * now we need to fold to ucs-2
                 */
                if (newch < 0x10000)
                {
                    --*outwords;
                    *(out++) = (WCHAR)newch;
                }
                else
                {
                    *outwords -= 2;
                    newch -= 0x10000;
                    *(out++) = (WCHAR)(0xD800 | (newch >> 10));
                    *(out++) = (WCHAR)(0xDC00 | (newch & 0x03FF));
                }
            }
        }
    }
    /* Buffer full 'errors' aren't errors, the client must inspect both
     * the inbytes and outwords values
     */
    return rcode_ok;
}

static int convert_ucs2_to_utf8(const WCHAR *in, size_t *inwords, char *out, size_t *outbytes)
{
    int64_t newch, require;
    size_t need;
    char *invout;
    int ch;

    while (*inwords && *outbytes)
    {
        ch = (unsigned short)(*in++);
        if (ch < 0x80)
        {
            --*inwords;
            --*outbytes;
            *(out++) = (unsigned char)ch;
        }
        else
        {
            if ((ch & 0xFC00) == 0xDC00) {
                /* Invalid Leading ucs-2 Multiword Continuation Character
                 */
                return -1;
            }
            if ((ch & 0xFC00) == 0xD800) {
                /* Leading ucs-2 Multiword Character
                 */
                if (*inwords < 2) {
                    /* Missing ucs-2 Multiword Continuation Character
                     */
                    return -2;
                }
                if (((unsigned short)(*in) & 0xFC00) != 0xDC00) {
                    /* Invalid ucs-2 Multiword Continuation Character
                     */
                    return -1;
                }
                newch = (ch & 0x03FF) << 10 | ((unsigned short)(*in++) & 0x03FF);
                newch += 0x10000;
            }
            else {
                /* ucs-2 Single Word Character
                 */
                newch = ch;
            }
            /* Determine the absolute minimum utf-8 bytes required
             */
            require = newch >> 11;
            need = 1;
            while (require)
                require >>= 5, ++need;
            if (need >= *outbytes)
                break; /* Insufficient buffer */
            *inwords -= (need > 2) + 1;
            *outbytes -= need + 1;
            /* Compute the utf-8 characters in last to first order,
             * calculating the lead character length bits along the way.
             */
            ch = 0200;
            out += need + 1;
            invout = out;
            while (need--) {
                ch |= ch >> 1;
                *(--invout) = (unsigned char)(0200 | (newch & 0077));
                newch >>= 6;
            }
            /* Compute the lead utf-8 character and move the dest offset */
            *(--invout) = (unsigned char)(ch | newch);
        }
    }
    /* Buffer full 'errors' aren't errors, the client must inspect both
     * the inwords and outbytes values
     */
    return rcode_ok;
}


static int path_utf8_to_unicode(WCHAR* retstr, size_t retlen, const char* srcstr)
{
    /* TODO: The computations could preconvert the string to determine
     * the true size of the retstr, but that's a memory over speed
     * tradeoff that isn't appropriate this early in development.
     *
     * Allocate the maximum string length based on leading 4
     * characters of \\?\ (allowing nearly unlimited path lengths)
     * plus the trailing null, then transform /'s into \\'s since
     * the \\?\ form doesn't allow '/' path seperators.
     *
     * Note that the \\?\ form only works for local drive paths, and
     * \\?\UNC\ is needed UNC paths.
     */
    size_t srcremains = strlen(srcstr) + 1;
    WCHAR *t = retstr;
    int rv;

    /* This is correct, we don't twist the filename if it is will
     * definitely be shorter than 248 characters.  It merits some
     * performance testing to see if this has any effect, but there
     * seem to be applications that get confused by the resulting
     * Unicode \\?\ style file names, especially if they use argv[0]
     * or call the Win32 API functions such as GetModuleName, etc.
     * Not every application is prepared to handle such names.
     *
     * Note also this is shorter than MAX_PATH, as directory paths
     * are actually limited to 248 characters.
     *
     * Note that a utf-8 name can never result in more wide chars
     * than the original number of utf-8 narrow chars.
     */
    if (srcremains > 248) {
        if (srcstr[1] == ':' && (srcstr[2] == '/' || srcstr[2] == '\\')) {
            wcscpy(retstr, L"\\\\?\\");
            retlen -= 4;
            t += 4;
        }
        else if ((srcstr[0] == '/' || srcstr[0] == '\\')
            && (srcstr[1] == '/' || srcstr[1] == '\\')
            && (srcstr[2] != '?')) {
            /* Skip the slashes */
            srcstr += 2;
            srcremains -= 2;
            wcscpy(retstr, L"\\\\?\\UNC\\");
            retlen -= 8;
            t += 8;
        }
    }

    if ((rv = convert_utf8_to_ucs2(srcstr, &srcremains, t, &retlen))) {
        return (rv == -2) ? -1 : rv;
    }
    if (srcremains) {
        return 1;
    }
	for (; *t; ++t) {
		if (*t == L'/') {
			*t = L'\\';
		}
	}

    return rcode_ok;
}

static int path_unicode_to_utf8(char* retstr, size_t retlen, const WCHAR* srcstr)
{
    /* Skip the leading 4 characters if the path begins \\?\, or substitute
     * // for the \\?\UNC\ path prefix, allocating the maximum string
     * length based on the remaining string, plus the trailing null.
     * then transform \\'s back into /'s since the \\?\ form never
     * allows '/' path seperators, and always uses '/'s.
     */
    size_t srcremains = wcslen(srcstr) + 1;
    int rv;
    char *t = retstr;
    if (srcstr[0] == L'\\' && srcstr[1] == L'\\' &&
        srcstr[2] == L'?'  && srcstr[3] == L'\\') {
        if (srcstr[4] == L'U' && srcstr[5] == L'N' &&
            srcstr[6] == L'C' && srcstr[7] == L'\\') {
            srcremains -= 8;
            srcstr += 8;
            retstr[0] = '\\';
            retstr[1] = '\\';
            retlen -= 2;
            t += 2;
        }
        else {
            srcremains -= 4;
            srcstr += 4;
        }
    }

    if ((rv = convert_ucs2_to_utf8(srcstr, &srcremains, t, &retlen))) {
        return rv;
    }
    if (srcremains) {
        return -3;
    }
    return rcode_ok;
}

#else //_WIN64
#include <unistd.h>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>

int unlink(const char *);
#endif

static int rfile_dir_make(const char *path)
{
#if defined(_WIN32) || defined(_WIN64)
#ifdef file_system_unicode
	WCHAR wpath[MAX_PATH];
	int rv = path_utf8_to_unicode(wpath, sizeof(wpath) / sizeof(WCHAR), path);
	if (rv != rcode_ok) {
		return rv;
	}
	if (!CreateDirectoryW(wpath, NULL)) {
		return -1;//get_os_error();
	}
#else //file_system_ansi
	if (!CreateDirectory(path, NULL)) {
		return -1;//get_os_error();
	}
#endif //_WIN64
#else
    mode_t mode = 0;
    if (mkdir(path, mode) != 0) {
        return errno;
    }
#endif
	return rcode_ok;
}

int rfile_make_dir(const char *path, bool recursive)
{
    if (!recursive) {
        return rfile_dir_make(path); /* Try to make PATH right out */
    }

    int rv = rcode_ok;
    //AccessMode = 00 表示只判断是否存在，02 是否可执行，_AccessMode = 04 是否可写，06 是否可读
    int path_len = rstr_len(path);
    int full_len = path_len * 2;
    char* format_path = rstr_new(full_len);
    format_path = rstr_repl(path, format_path, full_len, "\\", "/");
    char** dirs = rstr_split(format_path, "/");
    rstr_reset(format_path);

    char* dir_cur = NULL;
    rstr_array_for(dirs, dir_cur) {
        if (rstr_eq(dir_cur, rstr_empty)) {
            continue;
        }

        rstr_cat(format_path, dir_cur, full_len);
        rstr_cat(format_path, rfile_seperator, full_len);
        if (access(format_path, 0) != 0) {
            rv = rfile_dir_make(format_path);
            if (rv != 0) {
                break;
            }
        }
    }

    rstr_array_free(dirs);
    rstr_free(format_path);

	return rv;
}

int rfile_remove_dir(const char *path)
{
#if defined(_WIN32) || defined(_WIN64)
#ifdef file_system_unicode
	WCHAR wpath[MAX_PATH];
	int rv = path_utf8_to_unicode(wpath, sizeof(wpath) / sizeof(WCHAR), path);
	if (rv != rcode_ok) {
		return rv;
	}
	if (!RemoveDirectoryW(wpath)) {
		return -1;//rerror_get_os_error();
	}
#else //file_system_ansi
	if (!RemoveDirectory(path)) {
		return -1;//rerror_get_os_error();
	}
#endif //file_system_unicode

#else //_WIN64
	if (rmdir(path) != 0) {
		return errno;
	}
#endif
	return rcode_ok;
}


int rfile_copy_file(const char *src, const char *dst) {
    FILE *sfp = NULL, *dfp = NULL;
    size_t c;
    char buf[4096];

    rassert_goto(src != NULL, "", 1);
    rassert_goto(dst != NULL, "", 1);

    sfp = fopen(src, "rb");
    rassert_goto(sfp == NULL, "unable to open for reading", 1);

    dfp = fopen(dst, "wb+");
    rassert_goto(dfp == NULL, "unable to open for writing", 1);

    while ((c = fread(buf, 1, sizeof(buf), sfp)) > 0)
    {
        rassert_goto(fwrite(buf, 1, c, dfp) != 0, "error writing to dest", 1);
    }

    rassert_goto(!fclose(sfp), "close source failed.", 1);
    sfp = NULL;

    rassert_goto(!fclose(dfp), "error flushing dest", 1);
    dfp = NULL;

    return rcode_ok;
exit1:
    if (sfp)
        fclose(sfp);
    if (dfp)
        fclose(dfp);
    return 1;
}

int rfile_move_file(const char *src, const char *dst) {
    int rc;

    rassert_goto(src != NULL, "", 1);
    rassert_goto(dst != NULL, "", 1);

#ifdef rfile_link_always
    rassert_goto(!((rc = link(src, dst)) < 0 && errno != EXDEV), "", 1);

    if (rc && errno == EXDEV) {
        rassert_goto(rfile_copy_file(src, dst) == 0, "", 1); /* copy */
    }
#else
    rattribute_unused(rc);

    rassert_goto(rfile_copy_file(src, dst) == 0, "", 1);
#endif

    rassert_goto(rfile_remove(src) == 0, "", 1);

    return rcode_ok;
exit1:
    return -1;
}

int rfile_remove(const char *file) {
    rassert_goto(file != NULL, "", 1);

    rassert_goto(unlink(file) >= 0, "unlink", 1);

    return rcode_ok;
exit1:
    return -1;
}

int rfile_format_path(char* file_path) {
    size_t path_len = 0;
    while (true) {
        path_len = rstr_len(file_path);
        if (file_path[path_len] == '\\') {
            file_path[path_len] = rstr_end;
            continue;
        }

        path_len = rstr_len(file_path);
        if (file_path[path_len] == '/') {
            file_path[path_len] = rstr_end;
            continue;
        }

        break;
    }
    return rcode_ok;
}

rlist_t* rdir_list(const char* path, bool only_file, bool sub_dir) {
    rlist_t* ret_list = rlist_new(raymalloc);

    rlist_init(ret_list);
    ret_list->malloc_node = raymalloc;
    ret_list->free_node = rayfree;
    ret_list->free_node_val = rayfree;
    ret_list->free_self = rayfree;

    size_t path_len = rstr_len(path);
    char* format_path = rstr_new(path_len + 3);
    rstr_init(format_path);
    rstr_cat(format_path, path, path_len);

    if (format_path[path_len] == '*') {
        format_path[path_len] = rstr_end;
    }

    rfile_format_path(format_path);//去掉末尾正/反斜杠

    //结尾统一加上 /
    path_len = rstr_len(format_path);
    format_path[path_len] = '/';
    format_path[path_len + 1] = rstr_end;

    char* root_path = rstr_cpy(format_path, 0);

#if defined(_WIN32) || defined(_WIN64)

    format_path[path_len + 1] = '*';//windows需要通配符
    format_path[path_len + 2] = rstr_end;

#ifdef file_system_unicode
    char file_full_name[file_path_len_max * 3 + 1];
    WCHAR win_file_path[MAX_PATH];
    WIN32_FIND_DATA file_find_data;
    //PCWSTR win_file_path = L"E:\\Temp\\TxGameDownload\\MobileGamePCShared\\*.conf";
    path_utf8_to_unicode(win_file_path, sizeof(win_file_path) / sizeof(WCHAR), format_path);
    HANDLE file_find_ret = FindFirstFileW(win_file_path, &file_find_data);//FindFirstFile没有自动转换成 W !!
    if (file_find_ret == INVALID_HANDLE_VALUE) {
        //ERROR_ACCESS_DENIED == GetLastError();
        rerror("Unable to scan unicode directory: %s, error: %lu \n", format_path, GetLastError());
        rgoto(0);
    }
    path_unicode_to_utf8(file_full_name, file_path_len_max * 3 + 1, file_find_data.cFileName);
    if (!rstr_eq(file_full_name, ".") && !rstr_eq(file_full_name, "..")) {
        rlist_rpush(ret_list, rstr_cpy(file_full_name, 0));
    }

    while (FindNextFileW(file_find_ret, &file_find_data))
    {
        path_unicode_to_utf8(file_full_name, file_path_len_max * 3 + 1, file_find_data.cFileName);
        if (!rstr_eq(file_full_name, ".") && !rstr_eq(file_full_name, "..")) {
            rlist_rpush(ret_list, rstr_cpy(file_full_name, 0));
        }
    }

    FindClose(file_find_ret);
 
#else //file_system_ansi

    WIN32_FIND_DATA file_find_data;
    HANDLE file_find_ret = FindFirstFileA(format_path, &file_find_data);
    if (file_find_ret == INVALID_HANDLE_VALUE) {
        rerror("Unable to scan ansi directory: %s, error: %lu \n", format_path, GetLastError());
        rgoto(0);
    }
    if (!rstr_eq(file_find_data.cFileName, ".") && !rstr_eq(file_find_data.cFileName, "..")) {
        rlist_rpush(ret_list, rstr_cpy(file_find_data.cFileName, 0));
    }

    while (FindNextFileA(file_find_ret, &file_find_data))
    {
        if (!rstr_eq(file_find_data.cFileName, ".") && !rstr_eq(file_find_data.cFileName, "..")) {
            rlist_rpush(ret_list, rstr_cpy(file_find_data.cFileName, 0));
        }
    }

    FindClose(file_find_ret);

#endif //file_system_unicode

#else /* linux */
    DIR *dir_ptr;
    struct dirent* ptr;
    if (!(dir_ptr = opendir(format_path))) {
        rerror("Unable to scan directory: %s\n", format_path);
        rgoto(0);
    }
    while ((ptr = readdir(dir_ptr)) != 0) {
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0) {
            rlist_rpush(ret_list, rstr_cpy(ptr->d_name, 0));
        }
    }
    closedir(dir_ptr);
#endif // WIN32

exit0:
    if (format_path) {
        rstr_free(format_path);
    }
    if (root_path) {
        rstr_free(root_path);
    }

    return ret_list;
}

char* rdir_get_exe_root() {
    char full_path[file_path_len_max + 1];

#if defined(_WIN32) || defined(_WIN64)

#ifdef file_system_unicode
    WCHAR win_file_path[MAX_PATH];
    GetModuleFileNameW(NULL, win_file_path, MAX_PATH);
    path_unicode_to_utf8(full_path, file_path_len_max + 1, win_file_path);
    char* path_last_part = strrchr(full_path, '\\');
    if (path_last_part) {
        path_last_part[1] = rstr_end;
    }
    return rstr_cpy(full_path, 0);
#else //file_system_ansi

    char* exe_path = rstr_new(MAX_PATH);
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    char* path_last_part = strrchr(exe_path, '\\');
    if (path_last_part) {
        path_last_part[1] = rstr_end;
}   }
    return exe_path;
#endif

#else //_WIN64
    int rval;
    char* path_last_part;
    //size_t result_len;
    char* result;

    rval = readlink("/proc/self/exe", full_path, 4096);
    if (rval < 0 || rval >= 1024)
    {
        return rstr_empty;
    }
    full_path[rval] = '\0';
    path_last_part = strrchr(full_path, '/');
    if (path_last_part) {
        path_last_part[1] = rstr_end;
    }

    //result_len = path_last_part - full_path;
    //result = rstr_new(result_len + 2);
    result = rstr_cpy(full_path, 0);

    //strncpy(result, full_path, result_len);
    //result[result_len] = '/';
    //result[result_len + 1] = rstr_end;

    return result;
#endif
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif //__GNUC__