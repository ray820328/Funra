﻿/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#include "stdlib.h"

#include "rcommon.h"
#include "rstring.h"
#include "rfile.h"
#include "rtime.h"
#include "rlog.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif //__GNUC__

#define file_serail_num_len 20

rattribute_unused(static volatile int64_t time_last = 0);

static rmutex_t rlog_mutex;
static rlog_t** rlog_all = NULL;

static bool rlog_force_flush = false;//false启用日志buffer
static int rlog_flush_max = 30000;//n秒刷一次缓存
static int rlog_rollback_size = 1;//rolling文件大小，单位 m

//static char* rlog_filepath_format = "${date}/funra_${level}_${index}.log";
static char* rlog_param_date = "${date}";//"${date}"
static char* rlog_param_file_index = "${index}";//"${index}"
static char* rlog_param_level = "${level}";//"${level}"
static char* rlog_param_time = "${time}";//"${time}"
static char* rlog_param_file_suffix = "log";//"log"
static char* rlog_param_file_suffix_gap = ".";// "."
static char* rlog_param_file_index_gap = "_";//"_" 形如：xxx_0.log
static char* rlog_param_file_index_default = "";//"" 形如：xxx_.log

static char* _rlog_format_filepath_template(const char* filepath_template) {
    char* rlog_filepath_format = rstr_cpy(filepath_template, 0);
    
    char* temp_path = NULL;
    int file_suffix = rstr_last_index(rlog_filepath_format, rlog_param_file_suffix_gap);

    if (rstr_index(rlog_filepath_format, rlog_param_file_index) < 0) {
        if (file_suffix < 0) {
            temp_path = rlog_filepath_format;
            rlog_filepath_format = rstr_join(rlog_filepath_format, rlog_param_file_index_gap, rlog_param_file_index, rstr_array_end);
            rstr_free(temp_path);
        }
        else {
            temp_path = rlog_filepath_format;
            temp_path[file_suffix] = rstr_end;
            rlog_filepath_format = rstr_join(temp_path, rlog_param_file_index_gap, rlog_param_file_index, 
                rlog_param_file_suffix_gap, temp_path + file_suffix + 1, rstr_array_end);
            rstr_free(temp_path);
        }
    }

    if (file_suffix < 0) {
        temp_path = rlog_filepath_format;
        rlog_filepath_format = rstr_join(temp_path, rlog_param_file_suffix_gap, rlog_param_file_suffix, rstr_array_end);
        rstr_free(temp_path);
    }

    rfile_format_path(rlog_filepath_format);//路径格式化

    return rlog_filepath_format;
}
static char* _rlog_get_filepath(char* rlog_filepath_template, char* log_level_str, bool need_file_index) {
	char date_str_temp[16];
    char time_str_temp[16];
    char* filepath_str_temp = NULL;
    char* fileidx_str_temp = NULL;
    char* ret_str = NULL;
    //int fileidx_index = 0;
    int suffix_index = 0;

    rformat_time_s_yyyymmdd(date_str_temp, 0, 0);
    rformat_time_s_hhMMss(time_str_temp, 0, 0);

    filepath_str_temp = rstr_repl(rlog_filepath_template, rlog_param_date, date_str_temp);

    ret_str = rstr_repl(filepath_str_temp, rlog_param_time, time_str_temp);
    rstr_free(filepath_str_temp);
    filepath_str_temp = ret_str;

    ret_str = rstr_repl(filepath_str_temp, rlog_param_level, log_level_str);
    rstr_free(filepath_str_temp);
    filepath_str_temp = ret_str;

    //index，取当前目录同前缀文件下标，默认无
    int file_index_gap_len = (int)rstr_len(rlog_param_file_index_gap);
    char* path_name = rdir_get_path_dir(ret_str);
    char* file_name = rdir_get_path_filename(ret_str);
    char* file_prefix = rstr_sub(file_name, 0, rstr_last_index(file_name, rlog_param_file_index_gap) + file_index_gap_len, true);
    int file_prefix_len = (int)rstr_len(file_prefix);
    int file_id_max = -1;

    rassert(rdir_make(path_name, true) == rcode_ok, path_name);//确保目录存在

    if (need_file_index) {
        rlist_t* file_list = rdir_list(path_name, true, false);//dir_path
        rlist_iterator_t it = rlist_it(file_list, rlist_dir_tail);
        rlist_node_t* node = NULL;

		char* temp_file_name = NULL;
		int prefix_index = 0;
		int file_id = 0;

        while ((node = rlist_next(&it))) {
            temp_file_name = (char*)(node->val);
            prefix_index = rstr_index(temp_file_name, file_prefix);
            file_id = 0;
            if (prefix_index != 0) {//start with
                continue;
            }
            file_id_max = file_id_max == -1 ? 0 : file_id_max;

            suffix_index = rstr_index(temp_file_name + file_prefix_len, rlog_param_file_suffix_gap);//文件名后缀开始位置

            if (suffix_index > 0 && rstr_is_digit((const char*)(temp_file_name + file_prefix_len), suffix_index)) {
                temp_file_name[suffix_index] = rstr_end;
                file_id = rstr_2int(temp_file_name + file_prefix_len);
                file_id_max = file_id_max < file_id ? file_id : file_id_max;
            }
        }
        rlist_destroy(file_list);
    }

    if (file_id_max > -1) {
        rnum2str(fileidx_str_temp, file_id_max + 1, "%d");
    }
    else {
        fileidx_str_temp = rlog_param_file_index_default;
    }
    
    ret_str = rstr_repl(filepath_str_temp, rlog_param_file_index, fileidx_str_temp);
    rstr_free(filepath_str_temp);

    rstr_free(path_name);
    rstr_free(file_name);
    rstr_free(file_prefix);
    
    return ret_str;
}

static int _rlog_build_items(rlog_t* rlog, bool is_init, bool file_seperate) {
    rlog_info_t* log_item = NULL;
    int code_ret = 1;
    char* last_filepath = rstr_empty;//只支持两种，全散和单独一个文件
    char* roll_filepath = NULL;
    char* log_level_str = NULL;

    for (int cur_level = RLOG_VERB; cur_level < RLOG_ALL; ++cur_level) {
        if (is_init && rlog->log_items[cur_level]) {
            printf("log item already finished, level = %d.\n", cur_level);
            continue;//初始优先级更高，不覆盖
        }
        log_level_str = file_seperate ? rlog_level_2str(cur_level) : rlog_level_2str(RLOG_ALL);

        if (is_init) {
            log_item = rdata_new(rlog_info_t);
            memset(log_item, 0, sizeof(rlog_info_t));
            if (log_item == NULL) {
                printf("init log item failed, %s.\n", log_level_str);
                code_ret = 1;
                rgoto(1);
            }
            rlog->log_items[cur_level] = log_item;
            log_item->level = cur_level;

            //todo Ray ...
            log_item->item_buffer = rdata_new_buffer(rlog_temp_data_size);
            log_item->item_buffer[0] = '\0';
            log_item->buffer = rdata_new_buffer(rlog_cache_data_size);
            log_item->buffer[0] = '\0';
            log_item->item_fmt = rdata_new_buffer(rlog_temp_data_size);
            log_item->item_fmt[0] = '\0';
        }
        else {
            log_item = rlog->log_items[cur_level];
        }
        rassert(log_item != NULL, "");

        log_item->filename = _rlog_get_filepath(rlog->filepath_template, log_level_str, false);//初始都不带递增后缀
        rfile_format_path(log_item->filename);

        printf("build log item, filename = '%s'\n", log_item->filename);

        if (rstr_eq(last_filepath, rstr_empty) || !rstr_eq(last_filepath, log_item->filename)) {
            if (rfile_exists(log_item->filename)) {
                roll_filepath = _rlog_get_filepath(rlog->filepath_template, log_level_str, true);
                rfile_rename(log_item->filename, roll_filepath);
                rstr_free(roll_filepath);
            }

            last_filepath = log_item->filename;
            log_item->file_ptr = fopen(log_item->filename, "w+");
        }
        else {
            log_item->file_ptr = rlog->log_items[cur_level - 1]->file_ptr;
        }

        if (log_item->file_ptr == NULL) {
            printf("Cannot open file [%s], check file is opening or not!\n", log_item->filename);
            code_ret = 1;
            rgoto(1);
        }
    }

//exit0:
    code_ret = rcode_ok;

    printf("build rlog finished.\n");
exit1:
    if (code_ret != 0) {
        printf("build rlog failed. code = %d\n", code_ret);
    }

    return code_ret;
}


int rlog_init(const char* log_default_filename, const rlog_level_t log_default_level, const bool log_default_seperate_file, int file_size) {
    int ret_code = rcode_ok;
    rmutex_init(&rlog_mutex);

    rlog_t* rlog = rdata_new(rlog_t);
	memset(rlog, 0, sizeof(rlog_t));
    ret_code = rlog_init_log(rlog, log_default_filename, log_default_level, log_default_seperate_file, file_size);
	if (ret_code != rcode_ok) {
		rgoto(1);
	}

    rmutex_lock(&rlog_mutex);

    rlog_all = (rlog_t**)rdata_new_array(sizeof(rlog_t*), 2);
    rlog_all[0] = rlog;
    rlog_all[1] = NULL;

    rmutex_unlock(&rlog_mutex);
    
    ret_code = rcode_ok;

exit1:
    if (ret_code != rcode_ok && rlog != NULL) {
        rdata_free(rlog_t, rlog);
    }

    return ret_code;
}

int rlog_uninit() {
    int rcode = 0;
    rlog_t* rlog = NULL;
    rlog_info_t* log_item = NULL;

	for (int i = 0; rlog_all && rlog_all[i]; i++) {
        rlog = rlog_all[i];

        rcode = rlog_uninit_log(rlog);
        rassert(rcode == rcode_ok, "");

        rlog = NULL;
        rlog_all[i] = NULL;
    }

    rdata_free_array(rlog_all);

    rmutex_uninit(&rlog_mutex);

    return rcode_ok;
}

int rlog_init_log(rlog_t* rlog, const char* filename, const rlog_level_t level, const bool file_seperate, int file_size) {
    int code_ret = 1;

    if (rlog == NULL || rlog->inited) {
        code_ret = 1;
        rgoto(1);
    }

    rmutex_lock(&rlog_mutex);

    rlog->mutex = rdata_new(rmutex_t);
    rmutex_init(rlog->mutex);
    rlog->level = level == RLOG_ALL ? RLOG_VERB : level;
    rlog->file_seperate = file_seperate;
    rlog->file_size_max = file_size < rlog_rollback_size ? rlog_rollback_size * 1024000 : file_size * 1024000;

    if (level != RLOG_ALL && rlog->log_items[level]) {
        printf("rlog_init already inited, level = %d.\n", level);
        rgoto(0);
	}

	if (!filename || rstr_len(filename) > rlog_filename_length) {
        printf("rlog_init filename is too long.\n");
        code_ret = 1;
        rgoto(1);
	}

    rlog->filepath_template = _rlog_format_filepath_template(filename);

    code_ret = _rlog_build_items(rlog, true, file_seperate);

    if (code_ret != rcode_ok) {
        rgoto(1);
    }
	
exit0:
	rlog->inited = true;

    code_ret = rcode_ok;

    rinfo("init rlog finished (%p - %s)."Li, rlog, filename);
exit1:
    if (code_ret != 0) {
        rerror("init rlog failed (%s). code = %d"Li, filename, code_ret);
    }

    rmutex_unlock(&rlog_mutex);

    return code_ret;
}

int rlog_reset(rlog_t* rlog, const rlog_level_t level, int file_size) {

    return rcode_ok;
}

int rlog_uninit_log(rlog_t* rlog) {
	rinfo("uninit rlog (%p - %s)."Li, rlog, rlog->filepath_template);

	rmutex_lock(&rlog_mutex);

	rlog->inited = false;

    char* last_filepath = rstr_empty;//只支持两种，全散和单独一个文件

	for (int cur_level = RLOG_VERB; cur_level < RLOG_ALL; ++cur_level) {
        if (rlog->log_items[cur_level] != NULL) {
            if (rlog->log_items[cur_level]->file_ptr) {
                if (rstr_eq(last_filepath, rstr_empty) || !rstr_eq(last_filepath, rlog->log_items[cur_level]->filename)) {
                    last_filepath = rlog->log_items[cur_level]->filename;
                    fflush(rlog->log_items[cur_level]->file_ptr);
                    fclose(rlog->log_items[cur_level]->file_ptr);
                }
                rlog->log_items[cur_level]->file_ptr = NULL;
            }

            if (rlog->log_items[cur_level]->filename) {
                //printf("rlog_uninit free filename, p = %p, filename = %s\n", rlog->log_items[cur_level]->filename, rlog->log_items[cur_level]->filename);
                rayfree(rlog->log_items[cur_level]->filename);
                rlog->log_items[cur_level]->filename = NULL;
            }

            if (rlog->log_items[cur_level]->item_buffer) {
                rayfree(rlog->log_items[cur_level]->item_buffer);
                rlog->log_items[cur_level]->item_buffer = NULL;
            }

            if (rlog->log_items[cur_level]->buffer) {
                rayfree(rlog->log_items[cur_level]->buffer);
                rlog->log_items[cur_level]->buffer = NULL;
            }

            if (rlog->log_items[cur_level]->item_fmt) {
                rayfree(rlog->log_items[cur_level]->item_fmt);
                rlog->log_items[cur_level]->item_fmt = NULL;
            }

			rdata_free(rlog_info_t, rlog->log_items[cur_level]);
			rlog->log_items[cur_level] = NULL;
		}
	}

    rmutex_uninit(rlog->mutex);
    rdata_free(rmutex_t, rlog->mutex);

    rdata_free(rlog_t, rlog);

	rmutex_unlock(&rlog_mutex);

    return rcode_ok;
}

int rlog_flush_file(rlog_t* rlog, bool close_file) {
    bool last_flag = rlog_force_flush;

    rmutex_lock(rlog->mutex);

    rlog_force_flush = true;
	rlog_printf(NULL, RLOG_INFO, "log file flushed.\n");

    FILE* last_file = NULL;
	for (int cur_level = RLOG_VERB; cur_level < RLOG_ALL; ++cur_level) {
		if (rlog->log_items[cur_level] != NULL && rlog->log_items[cur_level]->file_ptr != NULL) {
            if (rlog->log_items[cur_level]->file_ptr != last_file) {
                fflush(rlog->log_items[cur_level]->file_ptr);
                if (close_file) {
                    fclose(rlog->log_items[cur_level]->file_ptr);
                    last_file = rlog->log_items[cur_level]->file_ptr;
                    rlog->log_items[cur_level]->file_ptr = NULL;
                }
            }
            else {
                if (close_file) {
                    rlog->log_items[cur_level]->file_ptr = NULL;
                }
            }
		}
	}

    rlog_force_flush = last_flag;

    rmutex_unlock(rlog->mutex);

    return rcode_ok;
}

int rlog_rolling_file(rlog_t* rlog, const rlog_level_t level) {
    int code_ret = 0;

	if (!rlog->inited) {
		rlog_printf(NULL, RLOG_INFO, "rolling rlog failed, not inited.\n");
		goto exit1;
	}

	rmutex_lock(&rlog_mutex);

	rlog_flush_file(rlog, true);//已经关闭了filepath对应的文件

    code_ret = _rlog_build_items(rlog, false, rlog->file_seperate);
    if (code_ret != rcode_ok) {
        rgoto(1);
    }
    
    code_ret = rcode_ok;
	printf("rolling rlog finished.\n");

exit1:
	rmutex_unlock(&rlog_mutex);

	return code_ret;
}

//日期会乱序
int rlog_printf_cached(rlog_t* rlog, rlog_level_t level, const char* fmt, ...) {
    rlog = rlog != NULL ? rlog : (rlog_all != NULL ? rlog_all[0] : NULL);

    if (rlog == NULL || !rlog->inited) {
        char* buffer_temp = (char*)malloc(rlog_temp_data_size);
        va_list print_params;
        va_start(print_params, fmt);
        vsnprintf(buffer_temp, rlog_temp_data_size - 1, fmt, print_params);
        va_end(print_params);
        printf("(rlog not ready) - %s", buffer_temp);
        free(buffer_temp);
        return 1;
    }
    if (level < rlog->level) {
        return rcode_ok;
    }
#ifdef print2file
    if (!unlikely(rlog->log_items[level])) {
        //va_list ap0;
        //va_start(ap0, fmt);
        //printf(fmt, ap0);
        //va_end(ap0);
        return -1;
    }
#endif

    rmutex_lock(rlog->mutex);

    static int64_t rlogFlushLast = 0;
    char* item_buffer = rlog->log_items[level]->item_buffer;
    char* buffer = rlog->log_items[level]->buffer;
    char* item_fmt = rlog->log_items[level]->item_fmt;
    char* log_level_str = rlog_level_2str(level);
    char timeStr[32];
    int64_t timeNow = rtime_millisec();
    rformat_time_s_full(timeStr, timeNow, 0);
    //get_cur_thread_id()
    strcat(item_fmt, timeStr);
    strcat(item_fmt, " [");
    strcat(item_fmt, log_level_str);//strupr(log_level_str)
    strcat(item_fmt, "] ");
    rstr_cat(item_fmt, fmt, sizeof(item_fmt));

    va_list ap;
    va_start(ap, fmt);
    int writeLen = vsnprintf(item_buffer, rlog_temp_data_size - 1, item_fmt, ap);
    va_end(ap);
    item_fmt[0] = '\0';

    if (writeLen >= rlog_temp_data_size) {
        item_buffer[rlog_temp_data_size - 1] = '\0';//rlog_temp_data_size定义不能小于64
		item_buffer[rlog_temp_data_size - 2] = '\n';
		item_buffer[rlog_temp_data_size - 3] = '.';
		item_buffer[rlog_temp_data_size - 4] = '.';
		item_buffer[rlog_temp_data_size - 5] = '.';
#ifdef print2file
		fprintf(rlog->log_items[level]->file_ptr, "%s%s\nlog item data exceed of max len(%d - %d).\n", buffer, item_buffer, rlog_temp_data_size, writeLen);
		fflush(rlog->log_items[level]->file_ptr);
#else
		printf("%s%s\nlog item data exceed of max len(%d - %d).\n", buffer, item_buffer, rlog_temp_data_size, writeLen);
#endif // print2file
		rlogFlushLast = timeNow;
		buffer[0] = '\0';
		item_buffer[0] = '\0';

        rmutex_unlock(&rlog_mutex);
        return 1;
	}

    static size_t freeLen = 0;
    freeLen = sizeof(buffer) - strlen(buffer) - 1;
    //printf("%%"PRId64" ms, len(%d - %d - %d) [%s]\n", (timeNow - rlogFlushLast), (int)strlen(buffer), (int)freeLen, (int)strlen(item_buffer), item_buffer);
    if (rlog_force_flush || ((timeNow - rlogFlushLast) > rlog_flush_max) || strlen(item_buffer) >= freeLen) {
#ifdef print2file
        fprintf(rlog->log_items[level]->file_ptr, "%s%s", buffer, item_buffer);
        fflush(rlog->log_items[level]->file_ptr);
#else
        printf("%s%s", buffer, item_buffer);
#endif // print2file
		rlogFlushLast = timeNow;
        buffer[0] = '\0';
		//printf("\nflush item finished...\n");

    }
    else {
        strcat(buffer, item_buffer);
    }
    item_buffer[0] = '\0';

    rmutex_unlock(rlog->mutex);
    return rcode_ok;
}

int rlog_printf(rlog_t* rlog, rlog_level_t level, const char* fmt, ...) {
    rlog = rlog != NULL ? rlog : (rlog_all != NULL ? rlog_all[0] : NULL);

    if (rlog == NULL || !rlog->inited) {
        char* buffer_temp = (char*)malloc(rlog_temp_data_size);
        va_list print_params;
        va_start(print_params, fmt);
        int len_temp = vsnprintf(buffer_temp, rlog_temp_data_size - 1, fmt, print_params);
        rassert(len_temp < rlog_temp_data_size, "");
        va_end(print_params);
        printf("(rlog not ready) - %s", buffer_temp);
        free(buffer_temp);
        return 1;
    }
    if (level < rlog->level) {
        return rcode_ok;
    }

    if (unlikely(rlog->log_items[level] == NULL)) {
        char* buffer_temp1 = (char*)malloc(rlog_temp_data_size);
        va_list print_params1;
        va_start(print_params1, fmt);
        int len_temp1 = vsnprintf(buffer_temp1, rlog_temp_data_size - 1, fmt, print_params1);
        rassert(len_temp1 < rlog_temp_data_size, "");
        va_end(print_params1);
        printf("(rlog item not ready, error!!!) - %s", buffer_temp1);
        free(buffer_temp1);
        return -1;
    }

    char* item_buffer = rlog->log_items[level]->item_buffer;
    //char* buffer = rlog->log_items[level]->buffer;
    //char* item_fmt = rlog->log_items[level]->item_fmt;
    char item_fmt[64] = { 0 };
    char* log_level_str = rlog_level_2str(level);
    char time_str[32];
    int64_t time_now = rtime_millisec();
    rformat_time_s_full(time_str, time_now, 0);
    strcat(item_fmt, time_str);
    strcat(item_fmt, " [");
    strcat(item_fmt, log_level_str);//strupr(log_level_str)
    strcat(item_fmt, "] %s");

    va_list ap;
    va_start(ap, fmt);
    int writeLen = vsnprintf(item_buffer, rlog_temp_data_size - 1, fmt, ap);
    va_end(ap);
    rassert(writeLen < rlog_temp_data_size, "");

#ifdef print2file
    fprintf(rlog->log_items[level]->file_ptr, item_fmt, item_buffer);
    //fflush(rlog->log_items[level]->file_ptr);
#else
    printf(item_fmt, item_buffer);
#endif // print2file
    item_fmt[0] = '\0';
    //item_buffer[0] = '\0';

    return rcode_ok;
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__