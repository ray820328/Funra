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
#include "rtime.h"
#include "rlog.h"
#include "rthread.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif //__GNUC__

#define file_serail_num_len 20

rattribute_unused(static volatile bool rlog_inited = false);
rattribute_unused(static volatile int64_t timeLast = 0);
rattribute_unused(static int64_t timeMax = 0);

static rmutex_t_def rlog_mutex;
static rlog_level_t rlog_level;
static bool file2seperate;

static bool rlog_force_flush = false;//false启用日志buffer
static int rlog_flush_max = 30000;//n秒刷一次缓存
rattribute_unused(static int rlog_rollback_size = 0);//rolling文件大小

static rlog_info_t* rlog_infos[RLOG_ALL];

//"XXX_%s_%s.log"
void rlog_init(const char* logFilename, const rlog_level_t logLevel, const bool seperateFile, const char* fileSuffix) {//字符长度都有保证
    if (rlog_inited) {
        return;
    }
    rmutex_init(&rlog_mutex);

    rmutex_lock(&rlog_mutex);

	rlog_level = logLevel == RLOG_ALL ? RLOG_VERB : logLevel;
	file2seperate = seperateFile;

    //char* filename[rlog_filename_length];
    char timeStr[32];
    int retCode = 1;
    char* last_filepath = rstr_empty;//只支持两种，全散和单独一个文件

    if (logLevel != RLOG_ALL && rlog_infos[logLevel]) {
		printf("rlog_init already inited, level = %d.\n", logLevel);
		goto Exit0;
	}

	if (!logFilename || strlen(logFilename) > rlog_filename_length - 32) {
		printf("rlog_init logFilename is too long.\n");
        goto Exit1;
	}

    if (!fileSuffix) {
        rformat_time_s_yyyymmddhhMMss(timeStr, 0);
    }
    else {
        if (sprintf(timeStr, "%s", fileSuffix) >= (int)sizeof(timeStr)) {
            printf("rlog_init logFilename is too long.\n");
            timeStr[(int)sizeof(timeStr) - 1] = '\0';
        }
    }
	//strcpy(filename, time_str);
	//strcat(filename, logFilename);

	for (int rLevel = RLOG_VERB; rLevel < RLOG_ALL; ++rLevel) {
		if (rlog_infos[rLevel]) {
			printf("log item already finished, level = %d.\n", rLevel);
			continue;//初始优先级更高，不覆盖
		}
		char* levelStr = RLOG_TOSTR(rLevel);
         //_strlwr(levelStr);

		rlog_info_t* log = raymalloc(sizeof(rlog_info_t));
		if (!log) {
			printf("rlog_init init failed, %s.\n", levelStr);
			exit(1);
		}
		rlog_infos[rLevel] = log;
		log->level = rLevel;

        size_t nSize = strlen(logFilename) + strlen(levelStr) + strlen(timeStr);
		log->filename = raymalloc(nSize + file_serail_num_len);//add suffix no. n digitals
		if (!log->filename) {
			printf("rlog_init init failed, %s.\n", levelStr);
            goto Exit1;
		}
		if (file2seperate) {
			//strcpy(log->filename, rlogStr);
			//strcat(log->filename, filename);
            sprintf(log->filename, logFilename, levelStr, timeStr);
            //printf("rlog_init init, p = %p, len = %d. size = %d, filename = '%s'\n", log->filename, strlen(log->filename), nSize, log->filename);
		}
		else {
            sprintf(log->filename, logFilename, RLOG_TOSTR(RLOG_ALL), timeStr);//all lt than level str defined
        }
        //todo Ray ...
        log->logItemData = raymalloc(rlog_temp_data_size);
        log->logItemData[0] = '\0';
        log->logData = raymalloc(rlog_cache_data_size);
        log->logData[0] = '\0';
        log->fmtDest = raymalloc(rlog_temp_data_size);
        log->fmtDest[0] = '\0';

        if (rstr_eq(last_filepath, rstr_empty) || !rstr_eq(last_filepath, log->filename)) {
            last_filepath = log->filename;
            log->file_ptr = fopen(log->filename, "w+");
        }
        else {
            log->file_ptr = rlog_infos[rLevel - 1]->file_ptr;
        }
		if (!log->file_ptr) {
			printf("Cannot open file [%s], check file is opening or not!\n", log->filename);
			goto Exit1;
		}
    }
	
Exit0:
	rlog_inited = true;

    retCode = 0;

	rlog_printf(RLOG_INFO, "init rlog finished.\n");
Exit1:
    if (retCode != 0) {
        printf("init rlog failed. code = %d\n", retCode);
    }

    rmutex_unlock(&rlog_mutex);
}
void rlog_uninit() {
	rlog_printf(RLOG_INFO, "uninit rlog finished.\n");

	rmutex_lock(&rlog_mutex);

	rlog_inited = false;

    char* last_filepath = rstr_empty;//只支持两种，全散和单独一个文件

	for (int rLevel = RLOG_VERB; rLevel < RLOG_ALL; ++rLevel) {
        if (rlog_infos[rLevel]) {
            if (rlog_infos[rLevel]->file_ptr) {
                if (rstr_eq(last_filepath, rstr_empty) || !rstr_eq(last_filepath, rlog_infos[rLevel]->filename)) {
                    last_filepath = rlog_infos[rLevel]->filename;
                    fflush(rlog_infos[rLevel]->file_ptr);
                    fclose(rlog_infos[rLevel]->file_ptr);
                }
                rlog_infos[rLevel]->file_ptr = NULL;
            }

            if (rlog_infos[rLevel]->filename) {
                //printf("rlog_uninit free filename, p = %p, filename = %s\n", rlog_infos[rLevel]->filename, rlog_infos[rLevel]->filename);
                rayfree(rlog_infos[rLevel]->filename);
                rlog_infos[rLevel]->filename = NULL;
            }

            if (rlog_infos[rLevel]->logItemData) {
                rayfree(rlog_infos[rLevel]->logItemData);
                rlog_infos[rLevel]->logItemData = NULL;
            }

            if (rlog_infos[rLevel]->logData) {
                rayfree(rlog_infos[rLevel]->logData);
                rlog_infos[rLevel]->logData = NULL;
            }

            if (rlog_infos[rLevel]->fmtDest) {
                rayfree(rlog_infos[rLevel]->fmtDest);
                rlog_infos[rLevel]->fmtDest = NULL;
            }

			rayfree(rlog_infos[rLevel]);
			rlog_infos[rLevel] = NULL;
		}
	}

	rmutex_unlock(&rlog_mutex);

	rmutex_uninit(&rlog_mutex);
}

int rlog_flush_file(bool close_file) {
    bool last_flag = rlog_force_flush;
    rlog_force_flush = true;
	rlog_printf(RLOG_INFO, "log file flushed.\n");
	for (int rLevel = RLOG_VERB; rLevel < RLOG_ALL; ++rLevel) {
		if (rlog_infos[rLevel] && rlog_infos[rLevel]->file_ptr) {
			fflush(rlog_infos[rLevel]->file_ptr);
			if (close_file) {
				fclose(rlog_infos[rLevel]->file_ptr);
				rlog_infos[rLevel]->file_ptr = NULL;
			}
		}
	}
    rlog_force_flush = last_flag;

    return rcode_ok;
}

int rlog_rolling_file() {
	rlog_info_t* log = NULL;
	char* temp_filename = NULL;
	bool changed = false;

	if (!rlog_inited) {
		rlog_printf(RLOG_INFO, "rolling rlog failed, not inited.\n");
		goto Exit1;
	}
	rmutex_lock(&rlog_mutex);

	rlog_flush_file(true);

	for (int cur_level = RLOG_VERB; cur_level < RLOG_ALL; ++cur_level) {
		log = rlog_infos[cur_level];
		if (!log) {
			continue;
		}

		temp_filename = rstr_cpy(log->filename, 0);
		temp_filename = rstr_repl(log->filename, temp_filename, rstr_len(temp_filename) + file_serail_num_len, ".txt", "_111.txt");
		
		if (file2seperate) {
			(void)rename(log->filename, temp_filename);
		}
		else if (!changed) {
			(void)rename(log->filename, temp_filename);
			changed = true;
		}

		log->file_ptr = fopen(log->filename, "w+");
		if (!log->file_ptr) {
			printf("Cannot open file [%s], check file is opening or not!\n", log->filename);
			goto Exit1;
		}
	}

//Exit0:
	rlog_inited = true;

	rlog_printf(RLOG_INFO, "rolling rlog finished.\n");

Exit1:
	rmutex_unlock(&rlog_mutex);

	return rcode_ok;
}

int rlog_printf_cached(rlog_level_t logLevel, const char* fmt, ...) {
    if (logLevel < rlog_level) {
        return rcode_ok;
    }
#ifdef print2file
    if (!unlikely(rlog_infos[logLevel])) {
        //va_list ap0;
        //va_start(ap0, fmt);
        //printf(fmt, ap0);
        //va_end(ap0);
        return -1;
    }
#endif

    rmutex_lock(&rlog_mutex);

    static int64_t rlogFlushLast = 0;
    char* logItemData = rlog_infos[logLevel]->logItemData;
    char* logData = rlog_infos[logLevel]->logData;
    char* fmtDest = rlog_infos[logLevel]->fmtDest;
    char* levelStr = RLOG_TOSTR(logLevel);
    char timeStr[32];
    int64_t timeNow = millisec_r();
    rformat_time_s_full(timeStr, timeNow);
    //get_cur_thread_id()
    strcat(fmtDest, timeStr);
    strcat(fmtDest, " [");
    strcat(fmtDest, levelStr);//strupr(levelStr)
    strcat(fmtDest, "] ");
    //size_t maxChars = sizeof(fmtDest) - strlen(fmtDest) - 1;
    //maxChars = maxChars > strlen(fmt) ? strlen(fmt) : maxChars;
    //strncat(fmtDest, fmt, maxChars);
    rstr_cat(fmtDest, fmt, sizeof(fmtDest));

    va_list ap;
    va_start(ap, fmt);
    int writeLen = vsnprintf(logItemData, rlog_temp_data_size - 1, fmtDest, ap);
    va_end(ap);
    fmtDest[0] = '\0';

    if (writeLen >= rlog_temp_data_size) {
        logItemData[rlog_temp_data_size - 1] = '\0';//rlog_temp_data_size定义不能小于64
		logItemData[rlog_temp_data_size - 2] = '\n';
		logItemData[rlog_temp_data_size - 3] = '.';
		logItemData[rlog_temp_data_size - 4] = '.';
		logItemData[rlog_temp_data_size - 5] = '.';
#ifdef print2file
		fprintf(rlog_infos[logLevel]->file_ptr, "%s%s\nlog item data exceed of max len(%d - %d).\n", logData, logItemData, rlog_temp_data_size, writeLen);
		fflush(rlog_infos[logLevel]->file_ptr);
#else
		printf("%s%s\nlog item data exceed of max len(%d - %d).\n", logData, logItemData, rlog_temp_data_size, writeLen);
#endif // print2file
		rlogFlushLast = timeNow;
		logData[0] = '\0';
		logItemData[0] = '\0';

        rmutex_unlock(&rlog_mutex);
        return 1;
	}

    static size_t freeLen = 0;
    freeLen = sizeof(logData) - strlen(logData) - 1;
    //printf("%%"PRId64" ms, len(%d - %d - %d) [%s]\n", (timeNow - rlogFlushLast), (int)strlen(logData), (int)freeLen, (int)strlen(logItemData), logItemData);
    if (rlog_force_flush || ((timeNow - rlogFlushLast) > rlog_flush_max) || strlen(logItemData) >= freeLen) {
#ifdef print2file
        fprintf(rlog_infos[logLevel]->file_ptr, "%s%s", logData, logItemData);
        fflush(rlog_infos[logLevel]->file_ptr);
#else
        printf("%s%s", logData, logItemData);
#endif // print2file
		rlogFlushLast = timeNow;
        logData[0] = '\0';
		//printf("\nflush item finished...\n");

    }
    else {
        strcat(logData, logItemData);
    }
    logItemData[0] = '\0';

    rmutex_unlock(&rlog_mutex);
    return rcode_ok;
}

int rlog_printf(rlog_level_t logLevel, const char* fmt, ...) {
    if (logLevel < rlog_level) {
        return rcode_ok;
    }
#ifdef print2file
    if (unlikely(rlog_infos[logLevel] == NULL)) {
        return -1;
    }
#endif

    rmutex_lock(&rlog_mutex);

    static int64_t rlogFlushLast = 0;
    char* logItemData = rlog_infos[logLevel]->logItemData;
    char* logData = rlog_infos[logLevel]->logData;
    char* fmtDest = rlog_infos[logLevel]->fmtDest;
    char* levelStr = RLOG_TOSTR(logLevel);
    char timeStr[32];
    int64_t timeNow = millisec_r();
    rformat_time_s_full(timeStr, timeNow);
    strcat(fmtDest, timeStr);
    strcat(fmtDest, " [");
    strcat(fmtDest, levelStr);//strupr(levelStr)
    strcat(fmtDest, "] %s");

    va_list ap;
    va_start(ap, fmt);
    int writeLen = vsnprintf(logItemData, rlog_temp_data_size - 1, fmt, ap);
    va_end(ap);
    rassert(writeLen < rlog_temp_data_size, "");

#ifdef print2file
    fprintf(rlog_infos[logLevel]->file_ptr, fmtDest, logItemData);
    //fflush(rlog_infos[logLevel]->file_ptr);
#else
    printf(fmtDest, logItemData);
#endif // print2file
    fmtDest[0] = '\0';
    //logItemData[0] = '\0';

    rmutex_unlock(&rlog_mutex);
    return rcode_ok;
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__