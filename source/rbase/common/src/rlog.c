/**
 * Copyright (c) 2014 ray
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author: Ray
 */

#ifndef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif //WIN32

#include "stdlib.h"
#include "string.h"

#include "rcommon.h"
#include "rtime.h"
#include "rlog.h"

__attribute__((unused)) static bool rlog_inited = false;
__attribute__((unused)) static int64_t timeLast;
__attribute__((unused)) static int64_t timeMax;

static bool rlog_force_flush = false;//false启用日志buffer
static int rlog_flush_max = 30000;//n秒刷一次缓存
__attribute__((unused)) static int rlog_rollback_size = 0;//rolling文件大小

static rlog_info_t* rlog_infos[RLOG_ALL];

//"XXX_%s_%s.log"
void init_rlog(const char* logFilename, const rlog_level_t logLevel, const bool seperateFile, const char* fileSuffix) {//字符长度都有保证
	if (logLevel != RLOG_ALL && rlog_infos[logLevel]) {
		printf("init_rlog already inited, level = %d.\n", logLevel);
		return;
	}

	if (!logFilename || strlen(logFilename) > rlog_filename_length - 32) {
		printf("init_rlog logFilename is too long.\n");
		exit(1);
	}

	//char* filename[rlog_filename_length];
	char time_str[32];
    if (!fileSuffix) {
        rformat_time_s_yyyymmddhhMMss(time_str, 0);
    }
    else {
        if (sprintf(time_str, "%s", fileSuffix) >= (int)sizeof(time_str)) {
            printf("init_rlog logFilename is too long.\n");
            time_str[(int)sizeof(time_str) - 1] = '\0';
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
			printf("init_rlog init failed, %s.\n", levelStr);
			exit(1);
		}
		rlog_infos[rLevel] = log;
		log->level = rLevel;

        size_t nSize = strlen(logFilename) + strlen(levelStr) + strlen(time_str);
		log->filename = raymalloc(nSize + 1);
		if (!log->filename) {
			printf("init_rlog init failed, %s.\n", levelStr);
			exit(1);
		}
		if (seperateFile) {
			//strcpy(log->filename, rlogStr);
   //         strcat(log->filename, filename);
            sprintf(log->filename, logFilename, levelStr, time_str);
            //printf("init_rlog init, p = %p, len = %d. size = %d, filename = '%s'\n", log->filename, strlen(log->filename), nSize, log->filename);
		}
		else {
            sprintf(log->filename, logFilename, "log", time_str);
        }
        //todo Ray ...
        log->logItemData = raymalloc(rlog_temp_data_size);
        log->logItemData[0] = '\0';
        log->logData = raymalloc(rlog_cache_data_size);
        log->logData[0] = '\0';
        log->fmtDest = raymalloc(rlog_temp_data_size);
        log->fmtDest[0] = '\0';

		log->file_ptr = fopen(log->filename, "a+");
		if (!log->file_ptr) {
			printf("Cannot open file [%s], check file is opening or not!\n", log->filename);
			return;
		}
	}
	
    rayprintf(RLOG_INFO, "init rlog finished.\n");
    return;
}
void uninit_rlog() {
	rayprintf(RLOG_INFO, "uninit rlog finished.\n");
	for (int rLevel = RLOG_VERB; rLevel < RLOG_ALL; ++rLevel) {
        if (rlog_infos[rLevel]) {
            if (rlog_infos[rLevel]->file_ptr) {
                fflush(rlog_infos[rLevel]->file_ptr);
                fclose(rlog_infos[rLevel]->file_ptr);
                rlog_infos[rLevel]->file_ptr = NULL;
            }

            if (rlog_infos[rLevel]->filename) {
                //printf("uninit_rlog free filename, p = %p, filename = %s\n", rlog_infos[rLevel]->filename, rlog_infos[rLevel]->filename);
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
}

int flush_rlog_file() {
    bool last_flag = rlog_force_flush;
    rlog_force_flush = true;
    rayprintf(RLOG_INFO, "logFile flushed.\n");
	for (int rLevel = RLOG_VERB; rLevel < RLOG_ALL; ++rLevel) {
		if (rlog_infos[rLevel] && rlog_infos[rLevel]->file_ptr) {
			fflush(rlog_infos[rLevel]->file_ptr);
			//fclose(logFilePtr);
		}
	}
    rlog_force_flush = last_flag;

    return 0;
}

int rayprintf(rlog_level_t logLevel, const char* fmt, ...) {
#ifdef print2file
    if (!unlikely(rlog_infos[logLevel])) {
        //va_list ap0;
        //va_start(ap0, fmt);
        //printf(fmt, ap0);
        //va_end(ap0);
        return -1;
    }
#endif

    static int64_t rlogFlushLast = 0;
    //static char logItemData[rlog_temp_data_size] = { '\0' };
    //static char logData[rlog_cache_data_size] = { '\0' };
    //static char fmtDest[rlog_temp_data_size] = { '\0' };
    char* logItemData = rlog_infos[logLevel]->logItemData;
    char* logData = rlog_infos[logLevel]->logData;
    char* fmtDest = rlog_infos[logLevel]->fmtDest;
    char* levelStr = RLOG_TOSTR(logLevel);
    static char timeStr[32];
    int64_t timeNow = millisec_r();
    rformat_time_s_full(timeStr, timeNow);
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

    return 0;
}
//int readFile(const char* filename) {
//    FILE *fp;
//    //文件以追加的方式打开
//    if ((fp = fopen(filename, "a+")) == NULL) {
//        printdbgr("Cannot open file, press any key to exit!\n");
//        return -1;
//    }
//    //while (!feof(fp))
//    //{
//    //    int a = 0;
//    //    int b = 0;
//    //    fscanf(fp, "%d + %d", &a, &b);
//    //    printdbgr("a = %d, b = %d\n", a, b);
//    //    //输出a = 1, b = 2
//    //}
//    //fclose(fp);
//    return 0;
//}
/**/

#ifndef WIN32
#pragma GCC diagnostic pop
#endif //WIN32