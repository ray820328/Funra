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
#pragma GCC diagnostic ignored "-Wnonnull"
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
#include <sys/stat.h>
#include <sys/types.h>

int unlink(const char *);
#endif


int rfile_create_in(const char *path, const char *filename) {
    char* file_path = rfile_get_filepath(path, filename);
    FILE* file;

    file = fopen(file_path, "r");
    if (file == NULL)
    {
        file = fopen(file_path, "w"); //使用“写入”方式创建文件 
    }

    rstr_free(file_path);

    fclose(file);

    return rcode_ok;
}
int rfile_create(const char* file_path) {
    FILE* file;

    file = fopen(file_path, "r");
    if (file == NULL)
    {
        file = fopen(file_path, "w"); //使用“写入”方式创建文件 
    }

    fclose(file);

    return rcode_ok;
}


int rfile_exists(const char* path) {
    if (path == NULL || rstr_eq(path, rstr_empty)) {
        return -1;
    }

    if (access(path, 0) == 0) {
        return 1;//不存在
    }

    return 0;//已存在
}

int rfile_init_item(rfile_item_t** file_item, char* filepath) {
    if (*file_item != NULL) {
        rerror("invalid file item");
        return rcode_invalid;
    }

    if (rfile_exists(filepath) != 0) {
        rerror("file not exists, path = %s", filepath);
        return rcode_invalid;
    }

    *file_item = rdata_new(rfile_item_t);
    rdata_init(*file_item, sizeof(rfile_item_t));

    (*file_item)->filename = rstr_cpy(filepath, 0);
    (*file_item)->state = rfile_state_init;

    return rcode_ok;
}

int rfile_uninit_item(rfile_item_t* file_item) {
    if (file_item == NULL || file_item->filename == NULL) {
        rerror("invalid file item");
        return rcode_invalid;
    }

    file_item->state = rfile_state_uninit;

    if (file_item->state == rfile_state_open) {
        rfile_close(file_item);
    }

    if (file_item->filename != NULL) {
        rstr_free(file_item->filename);
    }

    rdata_free(rfile_item_t, file_item);

    return rcode_ok;
}

// static int _rfile_skip_BOM (FILE* f) {
//     const char *p = "\xEF\xBB\xBF";  /* UTF-8 BOM mark */
//     int c;
//     do {
//         c = getc(f);
//         if (c == EOF || c != *(const unsigned char *)p++) {
//             return c;
//         }
//     } while (*p != '\0');

//     return getc(f);  /* return next character */
// }

// static int _rfile_skip_comment (FILE* f, int *cp) {
//     int c = *cp = _rfile_skip_BOM(f);
//     if (c == '#') {  /* first line is a comment (Unix exec. file)? */
//         do {  /* skip first line */
//             c = getc(f);
//         } while (c != EOF && c != '\n');
//         *cp = getc(f);  /* skip end-of-line, if present */
        
//         return 1;  /* there was a comment */
//     } else {
//         return 0;  /* no comment */
//     }
// }

int rfile_open(rfile_item_t* file_item, rfile_open_mode_t mode, bool binary) {
    if (file_item == NULL || file_item->filename == NULL) {
        rerror("invalid file item");
        return rcode_invalid;
    }

    if (file_item->state == rfile_state_open) {
        rerror("invalid file status, file = %s, state = %d", file_item->filename, file_item->state);
        return rcode_invalid;
    }

    char* open_op = NULL;
    switch(mode) {
        case rfile_open_mode_read:
            open_op = binary ? "rb" : "r";//rb 二进制 ,ccs=UTF-8
            break;
        case rfile_open_mode_write:
            open_op = binary ? "wb" : "w";
            break;
        case rfile_open_mode_read_write:
            open_op = binary ? "ab" : "a";
            break;
        case rfile_open_mode_append:
            open_op = binary ? "rb+" : "r+";
            break;
        case rfile_open_mode_overwrite:
            open_op = binary ? "wb+" : "w+";
            break;
        case rfile_open_mode_append_rw:
            open_op = binary ? "ab+" : "a+";
            break;
        default:
            open_op = binary ? "rb" : "r";
            break;
    }

    //freopen(file_item->filename, open_op)
    if ((file_item->file = fopen(file_item->filename, open_op)) == NULL) {
        rerror("open file failed, file = %s, state = %d, op = %s", 
            file_item->filename, file_item->state, open_op);
        return rcode_invalid;
    }

    file_item->state = rfile_state_open;

    return rcode_ok;
}

int rfile_close(rfile_item_t* file_item) {
    if (file_item == NULL || file_item->file == NULL) {
        rerror("invalid file item");
        return rcode_invalid;
    }

    if (file_item->state != rfile_state_open) {
        rerror("invalid file status, file = %s, state = %d", 
            file_item->filename == NULL ? "" : file_item->filename, file_item->state);
        return rcode_invalid;
    }

    fflush(file_item->file);
    fclose(file_item->file);
    file_item->file = NULL;

    file_item->state = rfile_state_close;

    return rcode_ok;
}

int rfile_read(rfile_item_t* file_item, char* data, int cache_size, int* real_size) {
   if (file_item == NULL || file_item->file == NULL) {
        rerror("invalid file item");
        return rcode_invalid;
    }

    if (file_item->state != rfile_state_open) {
        rerror("invalid file status, file = %s, state = %d", 
            file_item->filename == NULL ? "" : file_item->filename, file_item->state);
        return rcode_invalid;
    }

    // *real_size = read(file_item->file, data, cache_size);
    *real_size = fread(data, cache_size, 1, file_item->file);

    return rcode_ok;
}

int rfile_write(rfile_item_t* file_item, char* data, int buffer_size, int* real_size) {
    if (file_item == NULL || file_item->file == NULL) {
        rerror("invalid file item");
        return rcode_invalid;
    }

    if (file_item->state != rfile_state_open) {
        rerror("invalid file status, file = %s, state = %d", 
            file_item->filename == NULL ? "" : file_item->filename, file_item->state);
        return rcode_invalid;
    }

    buffer_size = buffer_size <= 0 ? rstr_len(data) : buffer_size;
    // *real_size = write(file_item->file, data, buffer_size);
    *real_size = fwrite(data, buffer_size, 1, file_item->file);

    return rcode_ok;
}

int rfile_copy_file(const char *src, const char *dst) {
    FILE *sfp = NULL, *dfp = NULL;
    size_t count;
    char buf[4096];

    rassert_goto(src != NULL, "", 1);
    rassert_goto(dst != NULL, "", 1);

    sfp = fopen(src, "rb");
    rassert_goto(sfp == NULL, "unable to open for reading", 1);

    dfp = fopen(dst, "wb+");//
    rassert_goto(dfp == NULL, "unable to open for writing", 1);

    while ((count = fread(buf, 1, sizeof(buf), sfp)) > 0)
    {
        rassert_goto(fwrite(buf, 1, count, dfp) != 0, "error writing to dest", 1);
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
    rassert_goto(src != NULL, "", 1);
    rassert_goto(dst != NULL, "", 1);

#ifdef rfile_link_always
    rassert_goto(!((rc = link(src, dst)) < 0 && errno != EXDEV), "", 1);

    if (rc && errno == EXDEV) {
        rassert_goto(rfile_copy_file(src, dst) == 0, "", 1); /* copy */
    }
#else

    rassert_goto(rfile_copy_file(src, dst) == 0, "", 1);
#endif

    rassert_goto(rfile_remove(src) == 0, "", 1);

    return rcode_ok;
exit1:
    return -1;
}

int rfile_rename(const char* src, const char* dst) {
    return rename(src, dst);
}

int rfile_remove(const char *file) {
    rassert_goto(file != NULL, "", 1);

    rassert_goto(unlink(file) >= 0, "unlink", 1);

    return rcode_ok;
exit1:
    return -1;
}

long rfile_get_size(const char* filename) {
    FILE* fp;
    long size;

    if ((fp = fopen(filename, "rb")) == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);

    return size;
}


static int _rdir_make_self(const char *path) {
	if (access(path, 0) == 0) {
		return rcode_ok;
	}
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
        return rerror_get_os_err();
    }
#endif
    return rcode_ok;
}

int rdir_make(const char* path, bool recursive) {
    if (path == NULL || rstr_eq(path, rstr_empty)) {
        return rcode_ok;
    }
    if (!recursive) {
        return _rdir_make_self(path); /* Try to make PATH right out */
    }

    int rv = rcode_ok;
    //AccessMode = 00 表示只判断是否存在，02 是否可执行，_AccessMode = 04 是否可写，06 是否可读
    char* temp_path = NULL;
    if (rstr_index(path, "\\\\") > -1) {
        temp_path = rstr_repl(path, "\\\\", "/");
    }
    char* temp_path2 = NULL;
    if (rstr_index(temp_path == NULL ? path : temp_path, "\\") > -1) {
        temp_path2 = rstr_repl(temp_path == NULL ? path : temp_path, "\\", "/");
    }

    char* dest_path = temp_path2 ? temp_path2 : (temp_path ? temp_path : (char*)path);
    char** dirs = rstr_split(dest_path, "/");
    int path_len = rstr_len(dest_path);

    rstr_free(temp_path);
    rstr_free(temp_path2);

	if (dirs != NULL) {
		char* format_path = rstr_new(path_len);
		rstr_reset(format_path);

		char* dir_cur = NULL;
		rstr_array_for(dirs, dir_cur) {
			if (rstr_eq(dir_cur, rstr_empty)) {
				continue;
			}

			rstr_cat(format_path, dir_cur, path_len);
			rstr_cat(format_path, rfile_seperator, path_len);
			
			rv = _rdir_make_self(format_path);
			if (rv != 0) {
				break;
			}
		}

		rstr_array_free(dirs);
		rstr_free(format_path);
	}
	else {
		rv = _rdir_make_self(dest_path);
	}

    return rv;
}

int rdir_remove(const char *path) {
#if defined(_WIN32) || defined(_WIN64)
#ifdef file_system_unicode
    WCHAR wpath[MAX_PATH];
    int rv = path_utf8_to_unicode(wpath, sizeof(wpath) / sizeof(WCHAR), path);
    if (rv != rcode_ok) {
        return rv;
    }
    if (!RemoveDirectoryW(wpath)) {
        DWORD errorno = rerror_get_os_err();
        return (int)errorno;
    }
#else //file_system_ansi
    if (!RemoveDirectory(path)) {
        return -1;//rerror_get_os_error();
    }
#endif //file_system_unicode

#else //_WIN64
    if (rmdir(path) != 0) {
        return rerror_get_os_err();
    }
#endif
    return rcode_ok;
}

char* rdir_get_path_dir(char* dir) {
	if (dir == NULL || rstr_eq(dir, rstr_empty)) {
		return rstr_empty;
	}

	char* format_path = rstr_cpy(dir, 0);

	rfile_format_path(format_path);

	int index = rstr_last_index(format_path, rfile_seperator);
	if (index < 0) {
		rstr_free(format_path);
		return rstr_empty;
	}

	format_path[index] = rstr_end;

	return format_path;
}
char* rdir_get_path_filename(char* dir) {
	if (dir == NULL) {
		return rstr_empty;
	}

	char* format_path = rstr_cpy(dir, 0);

	rfile_format_path(format_path);

	int index = rstr_last_index(format_path, rfile_seperator);
	if (index < 0) {
		return format_path;
	}

	char* ret_path = rstr_cpy(format_path + index + 1, 0);
	rstr_free(format_path);

	return ret_path;
}

int rfile_format_path(char* file_path) {
	char* format_str = file_path;
	char* p = NULL;
	char* q = NULL;

	int index = rstr_index(format_str,"\\");
	while (index >= 0) {
		p = format_str + index;
		*p = rfile_seperator[0];
		format_str = p + 1;

		if (format_str[0] == '\\') {
			p = format_str;
			q = p + 1;
			while (p[0] != rstr_end) {
				*p++ = *(q++);
			}
		}

		index = rstr_index(format_str, "\\");
	}

    size_t path_len = 0;
    while (true) {
        //path_len = rstr_len(file_path);
        //if (file_path[path_len] == '\\') {
        //    file_path[path_len] = rstr_end;
        //    continue;
        //}

        path_len = rstr_len(file_path);
        if (file_path[path_len - 1] == rfile_seperator[0]) {
            file_path[path_len - 1] = rstr_end;
            continue;
        }

        break;
    }
    return rcode_ok;
}

char* rfile_get_filepath(const char *path, const char *filename) {
    char* format_path = rstr_cpy(path, 0);
    rfile_format_path(format_path);

    rstr_array_make(paths, 3);
    paths[0] = format_path;
    paths[1] = (char*)filename;

    char* file_path = rstr_concat_array((const char**)paths, rfile_seperator, false);

    rstr_free(format_path);

    return file_path;
}

int _rdir_list(rlist_t* ret_list, const char* path, bool only_file, bool sub_dir) {
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
    char file_full_name[file_path_len_max * 3];
    WCHAR win_file_path[MAX_PATH];
    WIN32_FIND_DATA file_find_data;
    win_file_path[0] = rstr_end;
    path_utf8_to_unicode(win_file_path, sizeof(win_file_path) / sizeof(WCHAR), format_path);
    HANDLE file_find_ret = FindFirstFileW(win_file_path, &file_find_data);//FindFirstFile没有自动转换成 W !!
    if (file_find_ret == INVALID_HANDLE_VALUE) {
        //ERROR_ACCESS_DENIED == GetLastError();
        rerror("Unable to scan unicode directory: %s, error: %lu", format_path, GetLastError());
        rgoto(0);
    }
    path_unicode_to_utf8(file_full_name, file_path_len_max * 3 + 1, file_find_data.cFileName);
    if (!rstr_eq(file_full_name, rfile_path_current) && !rstr_eq(file_full_name, rfile_path_parent)) {
        if (file_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!only_file) {
                rlist_rpush(ret_list, file_full_name);
            }
            if (sub_dir) {
                _rdir_list(ret_list, file_full_name, only_file, sub_dir);
            }
        }
        else {
            rlist_rpush(ret_list, file_full_name);
        }
    }

    while (FindNextFileW(file_find_ret, &file_find_data))
    {
        file_full_name[0] = rstr_end;
        path_unicode_to_utf8(file_full_name, file_path_len_max * 3 + 1, file_find_data.cFileName);
        if (!rstr_eq(file_full_name, rfile_path_current) && !rstr_eq(file_full_name, rfile_path_parent)) {
            if (file_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!only_file) {
                    rlist_rpush(ret_list, file_full_name);
                }
                if (sub_dir) {
                    _rdir_list(ret_list, file_full_name, only_file, sub_dir);
                }
            }
            else {
                rlist_rpush(ret_list, file_full_name);
            }
        }
    }

    win_file_path[0] = rstr_end;
    file_full_name[0] = rstr_end;

#else //file_system_ansi

    WIN32_FIND_DATA file_find_data;
    HANDLE file_find_ret = FindFirstFileA(format_path, &file_find_data);
    if (file_find_ret == INVALID_HANDLE_VALUE) {
        rerror("Unable to scan ansi directory: %s, error: %lu", format_path, GetLastError());
        rgoto(0);
    }
    if (!rstr_eq(file_find_data.cFileName, rfile_path_current) && !rstr_eq(file_find_data.cFileName, rfile_path_parent)) {
        if (file_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!only_file) {
                rlist_rpush(ret_list, file_find_data.cFileName);
            }
            if (sub_dir) {
                _rdir_list(ret_list, file_find_data.cFileName, only_file, sub_dir);
            }
        }
        else {
            rlist_rpush(ret_list, file_find_data.cFileName);
        }
    }

    while (FindNextFileA(file_find_ret, &file_find_data))
    {
        if (!rstr_eq(file_find_data.cFileName, rfile_path_current) && !rstr_eq(file_find_data.cFileName, rfile_path_parent)) {
            if (file_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!only_file) {
                    rlist_rpush(ret_list, file_find_data.cFileName);
                }
                if (sub_dir) {
                    _rdir_list(ret_list, file_find_data.cFileName, only_file, sub_dir);
                }
            }
            else {
                rlist_rpush(ret_list, file_find_data.cFileName);
            }
        }
    }

#endif //file_system_unicode

#else /* linux */
    DIR* dir_ptr;
    struct dirent* ptr;
    //struct stat sb;
    //if (stat(file->d_name, &sb) >= 0 && S_ISDIR(sb.st_mode))

    if (!(dir_ptr = opendir(format_path))) {
        rerror("Unable to scan directory: %s", format_path);
        rgoto(0);
    }
    while ((ptr = readdir(dir_ptr)) != 0) {
        if (!rstr_eq(ptr->d_name, rfile_path_current) && !rstr_eq(ptr->d_name, rfile_path_parent)) {
            if (ptr->d_type == DT_DIR) {
                if (!only_file) {
                    rlist_rpush(ret_list, ptr->d_name);
                }
                if (sub_dir) {
                    _rdir_list(ret_list, ptr->d_name, only_file, sub_dir);
                }
            } else {
                rlist_rpush(ret_list, ptr->d_name);
            }
        }
    }
    
#endif // WIN32

exit0:

#if defined(_WIN32) || defined(_WIN64)
    FindClose(file_find_ret);
#else /* linux */
    closedir(dir_ptr);
#endif // WIN32

    if (format_path) {
        rstr_free(format_path);
    }
    if (root_path) {
        rstr_free(root_path);
    }

    return rcode_ok;
}

/** 保留结构，node可能包含详细信息 **/
rlist_t* rdir_list(const char* path, bool only_file, bool sub_dir) {
    rlist_t* ret_list = NULL;
    rlist_init(ret_list, rdata_type_string);

    path = (path == NULL || rstr_eq(path, rstr_empty)) ? rfile_path_current : path;

    _rdir_list(ret_list, path, only_file, sub_dir);

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
    int rval = 0;
    char* path_last_part = NULL;
    //size_t result_len;
    char* result = NULL;

    rval = readlink("/proc/self/exe", full_path, file_path_len_max);
    if (rval < 0 || rval >= file_path_len_max)
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