/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef RFILE_H
#define RFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rcommon.h"
#include "rlist.h"

/* ------------------------------- Macros ------------------------------------*/

#define file_path_len_max 2048 
#define rfile_seperator "/"
#define rfile_path_current "."
#define rfile_path_parent ".."

/* ------------------------------- APIs ------------------------------------*/

int rfile_create(const char *file_path);
int rfile_create_in(const char *dir, const char *filename);
int rfile_exists(const char* path);

int rfile_copy_file(const char* src, const char* dst);

int rfile_move_file(const char* src, const char* dst);
int rfile_rename(const char* src, const char* dst);

int rfile_remove(const char* file);

//int rfile_open(const char* file_path);
//int rfile_close(const char* file_path);

/** 不带后缀，形如：/temp/test **/
int rfile_format_path(char* file);

char* rfile_get_filepath(const char *path, const char *filename);

int rdir_make(const char* path, bool recursive);

int rdir_remove(const char *path);

rlist_t* rdir_list(const char* dir, bool only_file, bool sub_dir);

char* rdir_get_exe_root();

char* rdir_get_path_dir(char* dir);
char* rdir_get_path_filename(char* dir);


#ifdef __cplusplus
}
#endif

#endif //RFILE_H
